cmake_minimum_required(VERSION 3.14)
project(xsystem4 LANGUAGES C)
set(CMAKE_C_STANDARD 11)

include(FetchContent)

FetchContent_Declare(
  cglm
  URL https://github.com/recp/cglm/archive/refs/tags/v0.8.5.tar.gz
  URL_HASH SHA1=d387b0fccf6a87b7139115c8ebfc87fa2433b3f0
)
FetchContent_Declare(
  SDL
  URL https://github.com/libsdl-org/SDL/releases/download/release-2.26.1/SDL2-2.26.1.tar.gz
  URL_HASH SHA1=08a8cec21ffcebf44e4633a786d594a7811100d2
)
FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://github.com/freetype/freetype.git
  GIT_TAG VER-2-12-1
)
FetchContent_MakeAvailable(cglm SDL freetype)

add_library(ffi STATIC IMPORTED)
set_target_properties(ffi PROPERTIES
  IMPORTED_LOCATION ${CMAKE_STAGING_PREFIX}/lib/libffi.a
  INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_STAGING_PREFIX}/include)

# CMake < 2.23 does not look for packages in CMAKE_STAGING_PREFIX by default.
# https://gitlab.kitware.com/cmake/cmake/-/issues/16870
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE BOTH)
find_package(SndFile REQUIRED)

add_subdirectory(subprojects/libsys4)

add_library(xsystem4 SHARED)

target_compile_definitions(xsystem4 PRIVATE
  _DEFAULT_SOURCE
  USE_GLES
  )

target_include_directories(xsystem4 PRIVATE include)

set(VCS_TAG "unknown version")  # FIXME
configure_file(src/version.h.in version.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

target_sources(xsystem4 PRIVATE
  src/audio.c
  src/audio_meta.c
  src/audio_mixer.c
  src/asset_manager.c
  src/cJSON.c
  src/draw.c
  src/effect.c
  src/ffi.c
  src/font_freetype.c
  src/font_fnl.c
  src/format.c
  src/hacks.c
  src/heap.c
  src/id_pool.c
  src/input.c
  src/page.c
  src/resume.c
  src/savedata.c
  src/scene.c
  src/sprite.c
  src/system4.c
  src/text.c
  src/util.c
  src/video.c
  src/vm.c

  src/3d/debug.c
  src/3d/model.c
  src/3d/parser.c
  src/3d/particle.c
  src/3d/reign.c
  src/3d/renderer.c

  src/dungeon/dgn.c
  src/dungeon/dtx.c
  src/dungeon/dungeon.c
  src/dungeon/map.c
  src/dungeon/polyobj.c
  src/dungeon/renderer.c
  src/dungeon/skybox.c
  src/dungeon/tes.c

  src/parts/construction.c
  src/parts/debug.c
  src/parts/input.c
  src/parts/motion.c
  src/parts/parts.c
  src/parts/render.c
  src/parts/text.c

  src/hll/ACXLoader.c
  src/hll/ADVSYS.c
  src/hll/AliceLogo.c
  src/hll/AliceLogo2.c
  src/hll/AliceLogo3.c
  src/hll/AliceLogo4.c
  src/hll/AliceLogo5.c
  src/hll/AnteaterADVEngine.c
  src/hll/BanMisc.c
  src/hll/Bitarray.c
  src/hll/CGManager.c
  src/hll/CharSpriteManager.c
  src/hll/ChipmunkSpriteEngine.c
  src/hll/ChrLoader.c
  src/hll/CommonSystemData.c
  src/hll/Confirm.c
  src/hll/Confirm2.c
  src/hll/Confirm3.c
  src/hll/CrayfishLogViewer.c
  src/hll/Cursor.c
  src/hll/Data.c
  src/hll/DataFile.c
  src/hll/DrawDungeon.c
  src/hll/DrawGraph.c
  src/hll/DrawMovie.c
  src/hll/DrawMovie2.c
  src/hll/DrawPluginManager.c
  src/hll/DrawSimpleText.c
  src/hll/File.c
  src/hll/File2.c
  src/hll/FileOperation.c
  src/hll/FillAngle.c
  src/hll/GoatGUIEngine.c
  src/hll/Gpx2Plus.c
  src/hll/GUIEngine.c
  src/hll/iarray.c
  src/hll/IbisInputEngine.c
  src/hll/InputDevice.c
  src/hll/InputString.c
  src/hll/KiwiSoundEngine.c
  src/hll/LoadCG.c
  src/hll/MainEXFile.c
  src/hll/MainSurface.c
  src/hll/MarmotModelEngine.c
  src/hll/Math.c
  src/hll/MapLoader.c
  src/hll/MenuMsg.c
  src/hll/MonsterInfo.c
  src/hll/MsgLogManager.c
  src/hll/MsgLogViewer.c
  src/hll/MsgSkip.c
  src/hll/OutputLog.c
  src/hll/PassRegister.c
  src/hll/PlayDemo.c
  src/hll/PlayMovie.c
  src/hll/ReignEngine.c
  src/hll/SACT2.c
  src/hll/SengokuRanceFont.c
  src/hll/SoundFilePlayer.c
  src/hll/StoatSpriteEngine.c
  src/hll/StretchHelper.c
  src/hll/SystemService.c
  src/hll/SystemServiceEx.c
  src/hll/Timer.c
  src/hll/Toushin3Loader.c
  src/hll/VSFile.c
  )

target_link_libraries(xsystem4 PRIVATE
  m z SDL2 freetype ffi GLESv3 cglm SndFile::sndfile sys4)

target_compile_options(xsystem4 PRIVATE -Wno-unused-parameter)

install(TARGETS xsystem4 SDL2 freetype)
