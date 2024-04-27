/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <stdatomic.h>
#include <assert.h>

#include <sndfile.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/wasm_worker.h>
#include <emscripten/webaudio.h>
#else
#include <SDL.h>
#endif

#include "system4.h"
#include "system4/archive.h"

#include "asset_manager.h"
#include "audio.h"
#include "mixer.h"
#include "xsystem4.h"

#define clamp(min_value, max_value, value) min(max_value, max(min_value, value))
#define muldiv(x, y, denom) ((int64_t)(x) * (int64_t)(y) / (int64_t)(denom))

/*
 * The actual mixer implementation is contained in this header.
 * The rest of this file implements a high level interface for managing mixer
 * channels (loading audio, starting/stopping, etc.)
 */
#define STS_MIXER_IMPLEMENTATION
#include "sts_mixer.h"

#ifdef __EMSCRIPTEN__
#define CHUNK_SIZE 128  // To match the fixed chunk size of audio worklet.
#else
#define CHUNK_SIZE 1024
#endif

struct fade {
	atomic_bool fading;
	bool stop;
	uint_least32_t start_pos;
	uint_least32_t frames;
	uint_least32_t elapsed;
	float start_volume;
	float end_volume;
};

struct channel {
	// archive data
	struct archive_data *dfile;
	int no;
	int mixer_no;

	// audio file data
	SNDFILE *file;
	SF_INFO info;
	sf_count_t offset;

	// stream data
	atomic_int voice;
	sts_mixer_stream_t stream;
	float data[CHUNK_SIZE * 2];

	// main thread read-only
	atomic_uint_least32_t frame;

	atomic_uint volume;
	atomic_bool swapped;
	uint_least32_t loop_start;
	uint_least32_t loop_end;
	atomic_uint loop_count;
	struct fade fade;
};

struct mixer {
	sts_mixer_t mixer;
	sts_mixer_stream_t stream;
	int voice;
	atomic_bool muted;
	float data[CHUNK_SIZE * 2];
	char *name;

	struct mixer *parent;
	struct mixer **children;
	int nr_children;
};

static struct mixer *master = NULL;
static struct mixer *mixers = NULL;
static int nr_mixers = 0;

#ifdef __EMSCRIPTEN__

emscripten_lock_t mixer_lock = EMSCRIPTEN_LOCK_T_STATIC_INITIALIZER;

static void lock_mixer(void)
{
	while (!emscripten_lock_try_acquire(&mixer_lock))
		emscripten_sleep(0);
}

static void unlock_mixer(void)
{
	emscripten_lock_release(&mixer_lock);
}

/*
 * The Audio Worklet callback.
 */
EM_BOOL audio_callback(int num_inputs, const AudioSampleFrame *inputs, int num_outputs, AudioSampleFrame *outputs, int num_params, const AudioParamFrame *params, void *user_data)
{
	assert(num_outputs == 1);
	float buf[128 * 2];
	// memory.atomic.wait cannot be used in audio worklets, so we have to use busyspin.
	emscripten_lock_busyspin_waitinf_acquire(&mixer_lock);
	memset(buf, 0, 128 * 2 * sizeof(float));
	sts_mixer_mix_audio(&master->mixer, buf, 128);
	emscripten_lock_release(&mixer_lock);

	for (int i = 0; i < 128; i++) {
		outputs[0].data[i] = buf[i * 2];
		outputs[0].data[128 + i] = buf[i * 2 + 1];
	}

	return EM_TRUE;
}

static void aw_processor_created(EMSCRIPTEN_WEBAUDIO_T audio_context, EM_BOOL success, void *user_data)
{
	if (!success) {
		WARNING("emscripten_create_wasm_audio_worklet_processor_async failed");
		return;
	}
	int output_channel_counts[1] = { 2 };

	EmscriptenAudioWorkletNodeCreateOptions options = {
		.numberOfInputs = 0,
		.numberOfOutputs = 1,
		.outputChannelCounts = output_channel_counts
	};

	EMSCRIPTEN_AUDIO_WORKLET_NODE_T node =
		emscripten_create_wasm_audio_worklet_node(
			audio_context, "xsystem4_mixer", &options, &audio_callback, 0);
	EM_ASM({
		const audioWorkletNode = emscriptenGetAudioObject($0);
		audioWorkletNode.connect(Module.shell.get_audio_dest_node());
	}, node);
}

static void aw_initialized(EMSCRIPTEN_WEBAUDIO_T audio_context, EM_BOOL success, void *user_data)
{
	if (!success) {
		WARNING("emscripten_start_wasm_audio_worklet_thread_async failed");
		return;
	}

	WebAudioWorkletProcessorCreateOptions opts = {
		.name = "xsystem4_mixer",
	};
	emscripten_create_wasm_audio_worklet_processor_async(
		audio_context, &opts, aw_processor_created, 0);
}

static void audio_device_init(void)
{
	static uint8_t aw_stack[32 * 1024];
	EMSCRIPTEN_WEBAUDIO_T context = EM_ASM_INT({
		const ctx = Module.shell.get_audio_dest_node().context;
		return emscriptenRegisterAudioObject(ctx);
	});
	emscripten_start_wasm_audio_worklet_thread_async(
		context, aw_stack, sizeof(aw_stack), aw_initialized, 0);
}

EM_JS_DEPS(xsys4_audio_deps, "$emscriptenRegisterAudioObject,$emscriptenGetAudioObject");

#else // __EMSCRIPTEN

static SDL_AudioDeviceID audio_device = 0;
#define lock_mixer() SDL_LockAudioDevice(audio_device)
#define unlock_mixer() SDL_UnlockAudioDevice(audio_device)

/*
 * The SDL2 audio callback.
 */
static void audio_callback(possibly_unused void *data, Uint8 *stream, int len)
{
	sts_mixer_mix_audio(&master->mixer, stream, len / (sizeof(float) * 2));
	if (master->muted) {
		memset(stream, 0, len);
	}
}

static void audio_device_init(void)
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		ERROR("SDL_Init failed: %s", SDL_GetError());
	SDL_AudioSpec have;
	SDL_AudioSpec want = {
		.format = AUDIO_F32,
		.freq = 44100,
		.channels = 2,
		.samples = CHUNK_SIZE,
		.callback = audio_callback,
	};
	audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
	SDL_PauseAudioDevice(audio_device, 0);
}

#endif

/*
 * Seek to the specified position in the stream.
 * Returns true if the seek succeeded, otherwise returns false.
 */
static bool cb_seek(struct channel *ch, uint_least32_t pos)
{
	if (pos > ch->info.frames) {
		pos = ch->info.frames;
	}
	sf_count_t r = sf_seek(ch->file, pos, SEEK_SET);
	if (r < 0) {
		WARNING("sf_seek failed");
		return false;
	}
	ch->frame = r;
	return true;
}

/*
 * Seek to loop start, if the stream should loop.
 * Returns true if the stream should loop, false if it should stop.
 */
static bool cb_loop(struct channel *ch)
{
	if (!cb_seek(ch, ch->loop_start) || ch->loop_count == 1) {
		return false;
	}
	if (ch->loop_count > 1) {
		ch->loop_count--;
	}
	return true;
}

/*
 * Callback to refill the stream's audio data.
 * Called from sys_mixer_mix_audio.
 */
static int cb_read_frames(struct channel *ch, float *out, sf_count_t frame_count, uint_least32_t *num_read)
{
	*num_read = 0;

	// handle case where chunk crosses loop point (seamless)
	// NOTE: it's assumed that the length of the loop is greater than the chunk length
	if (ch->frame + frame_count >= ch->loop_end) {
		// read frames up to loop_end
		*num_read = sf_readf_float(ch->file, out, ch->loop_end - ch->frame);
		// adjust parameters for later
		ch->frame += *num_read;
		out += *num_read;
		frame_count -= *num_read;
		// seek to loop_start
		if (!cb_loop(ch))
			return STS_STREAM_COMPLETE;
	} else if (ch->frame >= ch->loop_end) {
		// seek to loop_start
		if (!cb_loop(ch))
			return STS_STREAM_COMPLETE;
	}

	// read remaining data
	sf_count_t n = sf_readf_float(ch->file, out, frame_count);
	*num_read += n;
	ch->frame += n;
	out += *num_read;
	frame_count -= *num_read;

	// XXX: This *shouldn't* be necessary, but sometimes libsndfile seems to stop reading
	//      just before the end of file (i.e. ch->frame + frame_count is < ch->info.frames,
	//      yet sf_readf_float returns less that the requested amount of frames).
	//      Not sure if this is a bug in xsystem4 or libsndfile
	if (frame_count > 0) {
		if (!cb_loop(ch))
			return STS_STREAM_COMPLETE;
		*num_read += sf_readf_float(ch->file, out, frame_count);
	}

	return STS_STREAM_CONTINUE;
}

/*
 * Calculate the gain for a fade at a given sample.
 */
static float cb_calc_fade(struct fade *fade)
{
	if (fade->elapsed >= fade->frames)
		return fade->stop ? 0.0 : fade->end_volume;

	float progress = (float)fade->elapsed / (float)fade->frames;
	float delta_v = fade->end_volume - fade->start_volume;
	float gain = fade->start_volume + delta_v * progress;
	if (gain < 0.0) return 0.0;
	if (gain > 1.0) return 1.0;
	return gain;
}

static int refill_stream(sts_mixer_sample_t *sample, void *data)
{
	struct channel *ch = data;
	uint_least32_t frames_read;
	memset(ch->data, 0, sizeof(float) * sample->length);

	// read audio data from file
	int r = cb_read_frames(ch, ch->data, CHUNK_SIZE, &frames_read);

	// convert mono to stereo
	if (ch->info.channels == 1) {
		for (int i = CHUNK_SIZE-1; i >= 0; i--) {
			ch->data[i*2+1] = ch->data[i];
			ch->data[i*2] = ch->data[i];
		}
	}

	// reverse LR channels
	else if (ch->swapped) {
		for (int i = 0; i < CHUNK_SIZE; i++) {
			float tmp = ch->data[i*2];
			ch->data[i*2] = ch->data[i*2+1];
			ch->data[i*2+1] = tmp;
		}
	}

	// set gain for fade
	if (ch->fade.fading) {
		float gain = cb_calc_fade(&ch->fade);
		mixers[ch->mixer_no].mixer.voices[ch->voice].gain = gain;
		ch->volume = gain * 100.0;

		ch->fade.elapsed += frames_read;
		if (ch->fade.elapsed >= ch->fade.frames) {
			ch->fade.fading = false;
			ch->volume = ch->fade.end_volume * 100.0;
			if (ch->fade.stop) {
				cb_seek(ch, 0);
				r = STS_STREAM_COMPLETE;
			}
		}
	} else {
		float gain = ch->volume / 100.0;
		mixers[ch->mixer_no].mixer.voices[ch->voice].gain = gain;
	}

	if (r == STS_STREAM_COMPLETE) {
		ch->voice = -1;
	}

	return r;
}

static int refill_mixer(sts_mixer_sample_t *sample, void *data)
{
	struct mixer *mixer = data;
	sts_mixer_mix_audio(&mixer->mixer, &mixer->data, CHUNK_SIZE);
	if (mixer->muted) {
		memset(mixer->data, 0, sizeof(float) * sample->length);
	}

	return STS_STREAM_CONTINUE;
}

int channel_play(struct channel *ch)
{
	lock_mixer();
	if (ch->voice >= 0) {
		unlock_mixer();
		return 1;
	}
	memset(ch->data, 0, sizeof(ch->data));
	ch->voice = sts_mixer_play_stream(&mixers[ch->mixer_no].mixer, &ch->stream, 1.0f);
	unlock_mixer();
	return 1;
}

int channel_stop(struct channel *ch)
{
	lock_mixer();
	if (ch->voice < 0) {
		unlock_mixer();
		return 1;
	}
	cb_seek(ch, 0);
	sts_mixer_stop_voice(&mixers[ch->mixer_no].mixer, ch->voice);
	ch->voice = -1;
	unlock_mixer();
	return 1;
}

int channel_is_playing(struct channel *ch)
{
	return ch->voice >= 0;
}

int channel_set_loop_count(struct channel *ch, int count)
{
	lock_mixer();
	ch->loop_count = count;
	unlock_mixer();
	return 1;
}

int channel_get_loop_count(struct channel *ch)
{
	return ch->loop_count;
}

int channel_set_loop_start_pos(struct channel *ch, int pos)
{
	lock_mixer();
	ch->loop_start = pos;
	unlock_mixer();
	return 1;
}

int channel_set_loop_end_pos(struct channel *ch, int pos)
{
	lock_mixer();
	ch->loop_end = pos;
	unlock_mixer();
	return 1;
}

int channel_fade(struct channel *ch, int time, int volume, bool stop)
{
	if (!time && stop)
		return channel_stop(ch);

	lock_mixer();
	if (!time) {
		// XXX: Fade with time=0 is used to set volume. This needs to
		//      take effect immediately, not via the audio callback
		//      because the stream isn't necessarily playing yet.
		//      E.g. the below call sequence should fade from 0 -> 33:
		//
		//          SACT2.Music_Fade(ch, 0, 0, 0);
		//          SACT2.Music_Fade(ch, 33, 4000, 0);
		//          SACT2.Music_Play(ch);
		ch->fade.fading = false;
		ch->volume = max(0, min(100, volume));
	} else {
		ch->fade.fading = true;
		ch->fade.stop = stop;
		ch->fade.start_pos = ch->frame;
		ch->fade.start_volume = (float)ch->volume / 100.0;
		ch->fade.frames = muldiv(time, ch->info.samplerate, 1000);
		ch->fade.elapsed = 0;
		ch->fade.end_volume = clamp(0.0f, 1.0f, (float)volume / 100.0f);
	}
	unlock_mixer();
	return 1;
}

int channel_stop_fade(struct channel *ch)
{
	lock_mixer();
	// XXX: we need to set the volume to end_volume and potentially stop the
	//      stream here; better to let the callback do it
	ch->fade.elapsed = ch->fade.frames;
	unlock_mixer();
	return 1;
}

int channel_is_fading(struct channel *ch)
{
	return ch->fade.fading;
}

int channel_pause(possibly_unused struct channel *ch)
{
	// NOTE: not implemented in Sengoku Rance's SACT2.dll
	WARNING("channel_pause not implemented");
	return 0;
}

int channel_restart(possibly_unused struct channel *ch)
{
	// NOTE: not implemented in Sengoku Rance's SACT2.dll
	WARNING("channel_restart not implemented");
	return 0;
}

int channel_is_paused(possibly_unused struct channel *ch)
{
	return 0;
}

int channel_get_pos(struct channel *ch)
{
	return muldiv(ch->frame, 1000, ch->info.samplerate);
}

int channel_get_length(struct channel *ch)
{
	// FIXME: how is this different than channel_get_time_length?
	return muldiv(ch->info.frames, 1000, ch->info.samplerate);
}

int channel_get_sample_pos(struct channel *ch)
{
	return ch->frame;
}

int channel_get_sample_length(struct channel *ch)
{
	return ch->info.frames;
}

int channel_seek(struct channel *ch, int pos)
{
	// NOTE: SACT2.Music_Seek doesn't seem to do anything in Sengoku Rance...
	lock_mixer();
	int r = cb_seek(ch, muldiv(pos, ch->info.samplerate, 1000));
	unlock_mixer();
	return r;
}

int channel_reverse_LR(struct channel *ch)
{
	ch->swapped = !ch->swapped;
	return 1;
}

int channel_get_volume(struct channel *ch)
{
	return ch->volume;
}

int channel_get_time_length(struct channel *ch)
{
	return muldiv(ch->info.frames, 1000, ch->info.samplerate);
}

static sf_count_t channel_vio_get_filelen(void *data)
{
	return ((struct channel*)data)->dfile->size;
}

static sf_count_t channel_vio_seek(sf_count_t offset, int whence, void *data)
{
	struct channel *ch = data;
	switch (whence) {
	case SEEK_CUR:
		ch->offset += offset;
		break;
	case SEEK_SET:
		ch->offset = offset;
		break;
	case SEEK_END:
		ch->offset = ch->dfile->size + offset;
		break;
	}
	ch->offset = clamp(0, (sf_count_t)ch->dfile->size, ch->offset);
	return ch->offset;
}

static sf_count_t channel_vio_read(void *ptr, sf_count_t count, void *data)
{
	struct channel *ch = data;
	sf_count_t c = min(count, (sf_count_t)ch->dfile->size - ch->offset);
	memcpy(ptr, ch->dfile->data + ch->offset, c);
	ch->offset += c;
	return c;
}

static sf_count_t channel_vio_write(possibly_unused const void *ptr,
				    possibly_unused sf_count_t count,
				    possibly_unused void *user_data)
{
	ERROR("sndfile vio write not supported");
}

static sf_count_t channel_vio_tell(void *data)
{
	return ((struct channel*)data)->offset;
}

static SF_VIRTUAL_IO channel_vio = {
	.get_filelen = channel_vio_get_filelen,
	.seek = channel_vio_seek,
	.read = channel_vio_read,
	.write = channel_vio_write,
	.tell = channel_vio_tell
};

struct channel *channel_open(enum asset_type type, int no)
{
	// get file from archive
	struct archive_data *dfile = asset_get(type, no);
	if (!dfile) {
		WARNING("Failed to load %s %d", type == ASSET_SOUND ? "WAV" : "BGM", no);
		return NULL;
	}

	struct channel *ch = channel_open_archive_data(dfile);
	if (!ch) {
		WARNING("Failed to open %s %d", type == ASSET_SOUND ? "WAV" : "BGM", no);
		return NULL;
	}

	if (type == ASSET_SOUND) {
		struct wai *wai = wai_get(no);
		ch->volume = 100;
		ch->loop_start = 0;
		ch->loop_end = ch->info.frames;
		ch->loop_count = 1;
		ch->mixer_no = wai ? wai->channel : 1;
	} else {
		struct bgi *bgi = bgi_get(no);
		if (bgi) {
			ch->volume = clamp(0, 100, bgi->volume);
			ch->loop_start = clamp(0, ch->info.frames, bgi->loop_start);
			ch->loop_end = clamp(0, ch->info.frames, bgi->loop_end);
			ch->loop_count = max(0, bgi->loop_count);
			ch->mixer_no = clamp(0, nr_mixers, bgi->channel);
		} else {
			ch->loop_count = 0;
		}
	}
	ch->no = no;

	return ch;
}

struct channel *channel_open_archive_data(struct archive_data *dfile)
{
	struct channel *ch = xcalloc(1, sizeof(struct channel));
	ch->dfile = dfile;

	// open file
	ch->file = sf_open_virtual(&channel_vio, SFM_READ, &ch->info, ch);
	if (sf_error(ch->file) != SF_ERR_NO_ERROR) {
		WARNING("sf_open_virtual failed: %s", sf_strerror(ch->file));
		goto error;
	}
	if (ch->info.channels > 2) {
		WARNING("Audio file has more than 2 channels");
		goto error;
	}

	// create stream
	ch->stream.userdata = ch;
	ch->stream.callback = refill_stream;
	ch->stream.sample.frequency = ch->info.samplerate;
	ch->stream.sample.audio_format = STS_MIXER_SAMPLE_FORMAT_FLOAT;
	ch->stream.sample.length = CHUNK_SIZE * 2;
	ch->stream.sample.data = ch->data;
	ch->voice = -1;

	ch->volume = 100;
	ch->loop_start = 0;
	ch->loop_end = ch->info.frames;
	ch->loop_count = 1;
	ch->mixer_no = 0;

	ch->no = -1;
	return ch;

error:
	archive_free_data(dfile);
	free(ch);
	return NULL;
}

void channel_close(struct channel *ch)
{
	channel_stop(ch);
	sf_close(ch->file);
	archive_free_data(ch->dfile);
	free(ch);
}

#define SJIS_MASTER "\x83\x7d\x83\x58\x83\x5e\x81\x5b"
#define SJIS_VOICE  "\x89\xb9\x90\xba"

void mixer_init(void)
{
	// initialize mixer naming
	if (!config.mixer_nr_channels) {
		nr_mixers = 3;
		mixers = xcalloc(nr_mixers, sizeof(struct mixer));
		mixers[0].name = strdup("Music");
		mixers[1].name = strdup("Sound");
		mixers[2].name = strdup("Master");
	} else {
		// NOTE: Older games don't have an explicit master channel.
		//       We add an extra mixer in this case for consistency.
		int need_master = 1;
		if (!strcmp(config.mixer_channels[0], "Master") || !strcmp(config.mixer_channels[0], SJIS_MASTER)) {
			need_master = 0;
		}
		nr_mixers = config.mixer_nr_channels + need_master;
		mixers = xcalloc(nr_mixers, sizeof(struct mixer));
		for (unsigned i = 0; i < config.mixer_nr_channels; i++) {
			mixers[i].name = strdup(config.mixer_channels[i]);
		}
		if (need_master) {
			mixers[nr_mixers-1].name = strdup("Master");
			master = &mixers[nr_mixers-1];
		} else {
			master = &mixers[0];
		}
	}

	// initialize mixer hierarchy
	// NOTE: "Master" and "Voice" are special nodes; all channels following
	//       them in the list become their children. This appears to be
	//       hardcoded behavior in system 4.
	struct mixer *parent = master;
	for (int i = 0; i < nr_mixers; i++) {
		if (&mixers[i] == master)
			continue;
		mixers[i].parent = parent;
		parent->children = xrealloc_array(parent->children, parent->nr_children, parent->nr_children+1, sizeof(struct mixer*));
		parent->children[parent->nr_children++] = &mixers[i];
		if (!strcmp(mixers[i].name, "Voice") || !strcmp(mixers[i].name, SJIS_VOICE)) {
			parent = &mixers[i];
		}
	}

	// initialize mixers
	for (int i = 0; i < nr_mixers; i++) {
		sts_mixer_init(&mixers[i].mixer, 44100, STS_MIXER_SAMPLE_FORMAT_FLOAT);
		int volume = i < (int)config.mixer_nr_channels ? config.mixer_volumes[i] : config.default_volume;
		mixers[i].mixer.gain = clamp(0.0f, 1.0f, (float)volume / 100.0f);
	}

	// initialize mixer streams
	for (int i = 0; i < nr_mixers; i++) {
		if (&mixers[i] == master)
			continue;
		mixers[i].stream.userdata = &mixers[i];
		mixers[i].stream.callback = refill_mixer;
		mixers[i].stream.sample.frequency = 44100;
		mixers[i].stream.sample.audio_format = STS_MIXER_SAMPLE_FORMAT_FLOAT;
		mixers[i].stream.sample.length = CHUNK_SIZE * 2;
		mixers[i].stream.sample.data = mixers[i].data;
		mixers[i].voice = sts_mixer_play_stream(&mixers[i].parent->mixer, &mixers[i].stream, 1.0f);
	}

	// read audio metadata
	if (config.bgi_path)
		bgi_read(config.bgi_path);
	if (config.wai_path)
		wai_load(config.wai_path);

	audio_device_init();
}

int mixer_get_numof(void)
{
	// Return the number of mixers specified in System40.ini, even if
	// xsystem4 added more mixers.
	return config.mixer_nr_channels;
}

const char *mixer_get_name(int n)
{
	if (n < 0 || n >= config.mixer_nr_channels)
		return NULL;
	return mixers[n].name;
}

int mixer_set_name(int n, const char *name)
{
	if (n < 0 || n >= config.mixer_nr_channels)
		return 0;
	free(mixers[n].name);
	mixers[n].name = strdup(name);
	return 1;
}

int mixer_get_volume(int n, int *volume)
{
	if (n < 0 || n >= config.mixer_nr_channels) {
		return 0;
	}
	lock_mixer();
	*volume = clamp(0, 100, (int)(mixers[n].mixer.gain * 100));
	unlock_mixer();
	return 1;
}

int mixer_set_volume(int n, int volume)
{
	if (n < 0 || n >= config.mixer_nr_channels)
		return 0;
	lock_mixer();
	mixers[n].mixer.gain = clamp(0.0f, 1.0f, (float)volume / 100.0f);
	unlock_mixer();
	return 1;
}
int mixer_get_mute(int n, int *mute)
{
	if (n < 0 || n >= config.mixer_nr_channels)
		return 0;
	*mute = mixers[n].muted;
	return 1;
}

int mixer_set_mute(int n, int mute)
{
	if (n < 0 || n >= config.mixer_nr_channels)
		return 0;
	mixers[n].muted = !!mute;
	return 1;
}

int mixer_stream_play(sts_mixer_stream_t* stream, int volume)
{
	lock_mixer();
	float gain = clamp(0.0f, 1.0f, (float)volume / 100.0f);
	int voice = sts_mixer_play_stream(&master->mixer, stream, gain);
	unlock_mixer();
	return voice;
}

bool mixer_stream_set_volume(int voice, int volume)
{
	if (voice < 0 || voice >= STS_MIXER_VOICES)
		return false;
	lock_mixer();
	master->mixer.voices[voice].gain = clamp(0.0f, 1.0f, (float)volume / 100.0f);
	unlock_mixer();
	return true;
}

void mixer_stream_stop(int voice)
{
	lock_mixer();
	sts_mixer_stop_voice(&master->mixer, voice);
	unlock_mixer();
}
