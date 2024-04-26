/* Copyright (C) 2023 kichikuou <KichikuouChrome@gmail.com>
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

#include <assert.h>
#include <errno.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/wasm_worker.h>
#endif

#include "system4.h"
#include "system4/file.h"

#include "gfx/gfx.h"
#include "movie.h"
#include "mixer.h"
#include "sprite.h"
#include "sts_mixer.h"
#include "xsystem4.h"

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"

#ifdef __EMSCRIPTEN__

typedef emscripten_lock_t lock_t;

static lock_t *create_lock(void)
{
	lock_t *lock = malloc(sizeof(lock_t));
	emscripten_lock_init(lock);
	return lock;
}

static void destroy_lock(lock_t *lock)
{
	free((void*)lock);
}

static void acquire_lock_audio_thread(lock_t *lock)
{
	// memory.atomic.wait cannot be used in audio worklets, so we have to use busyspin.
	emscripten_lock_busyspin_waitinf_acquire(lock);
}

static void acquire_lock_main_thread(lock_t *lock)
{
	while (!emscripten_lock_try_acquire(lock))
		emscripten_sleep(0);
}

static void release_lock(lock_t *lock)
{
	emscripten_lock_release(lock);
}

#else

typedef SDL_mutex lock_t;

static lock_t *create_lock(void)
{
	return SDL_CreateMutex();
}

static void destroy_lock(lock_t *lock)
{
	if (lock)
		SDL_DestroyMutex(lock);
}

static void acquire_lock_audio_thread(lock_t *lock)
{
	SDL_LockMutex(lock);
}

static void acquire_lock_main_thread(lock_t *lock)
{
	SDL_LockMutex(lock);
}

static void release_lock(lock_t *lock)
{
	SDL_UnlockMutex(lock);
}

#endif

static Shader movie_shader;

struct movie_context {
	plm_t *plm;
	lock_t *lock;

	plm_samples_t * _Atomic pending_audio_frame;
	plm_frame_t *pending_video_frame;
	GLuint textures[3];  // Y, Cb, Cr

	sts_mixer_stream_t sts_stream;
	int voice;
	int volume;

	// Time keeping data. Written by audio handler and referenced by video
	// handler (i.e. we sync the video to the audio).
	atomic_int_least32_t stream_time_origin;  // wall clock - stream time (msec)
};

static int audio_callback(sts_mixer_sample_t *sample, void *data)
{
	struct movie_context *mc = data;
	assert(sample == &mc->sts_stream.sample);

	plm_samples_t *frame = mc->pending_audio_frame;
	if (!frame) {
		acquire_lock_audio_thread(mc->lock);
		if (!mc->pending_audio_frame) {
			mc->pending_audio_frame = plm_decode_audio(mc->plm);
		}
		frame = mc->pending_audio_frame;
		release_lock(mc->lock);
	}

	if (!frame) {
		free(sample->data);
		sample->length = 0;
		sample->data = NULL;
		mc->voice = -1;
		return STS_STREAM_COMPLETE;
	}
	sample->length = frame->count * 2;
	memcpy(sample->data, frame->interleaved, sample->length * sizeof(float));

	// Update the timestamp.
	mc->stream_time_origin = SDL_GetTicks() - frame->time * 1000;

	mc->pending_audio_frame = NULL;
	return STS_STREAM_CONTINUE;
}

static void prepare_movie_shader(struct gfx_render_job *job, void *data)
{
	struct movie_context *mc = data;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mc->textures[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, mc->textures[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, mc->textures[2]);
}

static void load_movie_shader()
{
	gfx_load_shader(&movie_shader, "shaders/render.v.glsl", "shaders/movie.f.glsl");
	glUseProgram(movie_shader.program);
	glUniform1i(glGetUniformLocation(movie_shader.program, "texture_y"), 0);
	glUniform1i(glGetUniformLocation(movie_shader.program, "texture_cb"), 1);
	glUniform1i(glGetUniformLocation(movie_shader.program, "texture_cr"), 2);
	movie_shader.prepare = prepare_movie_shader;
}

static void update_texture(GLuint unit, GLuint texture, plm_plane_t *plane)
{
	glActiveTexture(unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, plane->width, plane->height, 0,
	             GL_RED, GL_UNSIGNED_BYTE, plane->data);
}

#ifdef __EMSCRIPTEN__

EM_ASYNC_JS(void*, load_nonresident_file, (const char *path, size_t *len), {
	const data = await Module.shell.load_nonresident_file(UTF8ToString(path));
	if (!data)
		return 0;
	const ptr = Module._malloc(data.byteLength);
	Module.HEAPU8.set(data, ptr);
	Module.HEAPU32[len >> 2] = data.byteLength;
	return ptr;
});

#endif

struct movie_context *movie_load(const char *filename)
{
	struct movie_context *mc = xcalloc(1, sizeof(struct movie_context));
	mc->voice = -1;
	char *path = gamedir_path(filename);
#ifdef __EMSCRIPTEN__
	size_t len;
	void *data = load_nonresident_file(path, &len);
	if (!data) {
		WARNING("Cannot read %s", path);
		free(path);
		movie_free(mc);
		return NULL;
	}
	mc->plm = plm_create_with_memory(data, len, TRUE);
#else
	FILE *fp = file_open_utf8(path, "rb");
	if (!fp) {
		WARNING("%s: %s", path, strerror(errno));
		free(path);
		movie_free(mc);
		return NULL;
	}
	mc->plm = plm_create_with_file(fp, TRUE);
#endif
	if (!plm_has_headers(mc->plm)) {
		WARNING("%s: not a MPEG-PS file", path);
		free(path);
		movie_free(mc);
		return NULL;
	}
	free(path);

	if (!movie_shader.program)
		load_movie_shader();

	glGenTextures(3, mc->textures);
	for (int i = 0; i < 3; i++) {
		glBindTexture(GL_TEXTURE_2D, mc->textures[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	mc->lock = create_lock();
	mc->volume = 100;
	return mc;
}

void movie_free(struct movie_context *mc)
{
	if (mc->voice >= 0) {
		mixer_stream_stop(mc->voice);
		free(mc->sts_stream.sample.data);
	}

	if (mc->plm)
		plm_destroy(mc->plm);
	if (mc->textures[0])
		glDeleteTextures(3, mc->textures);
	destroy_lock(mc->lock);
	free(mc);
}

void prepare_audio_frame(struct movie_context *mc)
{
	if (mc->pending_audio_frame)
		return;
	acquire_lock_main_thread(mc->lock);
	if (!mc->pending_audio_frame)
		mc->pending_audio_frame = plm_decode_audio(mc->plm);
	release_lock(mc->lock);
}

bool movie_play(struct movie_context *mc)
{
	// Start the audio stream.
	mc->stream_time_origin = SDL_GetTicks();
	mc->sts_stream.userdata = mc;
	mc->sts_stream.callback = audio_callback;
	mc->sts_stream.sample.frequency = plm_get_samplerate(mc->plm);
	mc->sts_stream.sample.audio_format = STS_MIXER_SAMPLE_FORMAT_FLOAT;
	mc->sts_stream.sample.data = xmalloc(PLM_AUDIO_SAMPLES_PER_FRAME * 2 * sizeof(float));
	mc->voice = mixer_stream_play(&mc->sts_stream, mc->volume);
	prepare_audio_frame(mc);
	return true;
}

bool movie_draw(struct movie_context *mc, struct sact_sprite *sprite)
{
	// Decode a frame, unless we already have one.
	plm_frame_t *frame = mc->pending_video_frame;
	mc->pending_video_frame = NULL;
	if (!frame) {
		acquire_lock_main_thread(mc->lock);
		frame = plm_decode_video(mc->plm);
		release_lock(mc->lock);
		if (!frame)
			return plm_video_has_ended(mc->plm->video_decoder);
	}

	// If the frame's timestamp is in the future, save the frame and return.
	double now = (SDL_GetTicks() - mc->stream_time_origin) / 1000.0;
	if (frame->time > now) {
		mc->pending_video_frame = frame;
		prepare_audio_frame(mc);
		return true;
	}

	// Render the frame.
	update_texture(GL_TEXTURE0, mc->textures[0], &frame->y);
	update_texture(GL_TEXTURE1, mc->textures[1], &frame->cb);
	update_texture(GL_TEXTURE2, mc->textures[2], &frame->cr);

	float w, h;
	GLuint fbo;
	if (sprite) {
		struct texture *texture = sprite_get_texture(sprite);
		w = texture->w;
		h = texture->h;
		fbo = gfx_set_framebuffer(GL_DRAW_FRAMEBUFFER, texture, 0, 0, w, h);
	} else {
		// Draw directly to the main framebuffer.
		w = config.view_width;
		h = config.view_height;
		gfx_clear();
	}

	mat4 world_transform = MAT4(
	     w, 0, 0, 0,
	     0, h, 0, 0,
	     0, 0, 1, 0,
	     0, 0, 0, 1);
	mat4 wv_transform = WV_TRANSFORM(w, h);

	struct gfx_render_job job = {
		.shader = &movie_shader,
		.shape = GFX_RECTANGLE,
		.texture = 0,
		.world_transform = world_transform[0],
		.view_transform = wv_transform[0],
		.data = mc
	};
	gfx_render(&job);

	if (sprite) {
		gfx_reset_framebuffer(GL_DRAW_FRAMEBUFFER, fbo);
		sprite_dirty(sprite);
	} else {
		gfx_swap();
	}

	prepare_audio_frame(mc);
	return true;
}

bool movie_is_end(struct movie_context *mc)
{
	return plm_has_ended(mc->plm);
}

int movie_get_position(struct movie_context *mc)
{
	int ms = mc->stream_time_origin ? SDL_GetTicks() - mc->stream_time_origin : 0;
	return ms;
}

bool movie_set_volume(struct movie_context *mc, int volume)
{
	mc->volume = volume;
	if (mc->voice >= 0)
		mixer_stream_set_volume(mc->voice, volume);
	return true;
}
