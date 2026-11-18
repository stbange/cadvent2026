# SPDX-License-Identifier: MIT
# Provides the SDL3::SDL3 target: an installed SDL3 (CADV_SYSTEM_SDL3=ON) or a
# pinned release built from source as a static library. Offline builds can
# point FETCHCONTENT_SOURCE_DIR_SDL3 at an extracted SDL source tree.
if(CADV_SYSTEM_SDL3)
  find_package(SDL3 REQUIRED CONFIG)
  return()
endif()

include(FetchContent)
set(CADV_SDL3_VERSION 3.4.20)
FetchContent_Declare(SDL3
  URL https://github.com/libsdl-org/SDL/releases/download/release-${CADV_SDL3_VERSION}/SDL3-${CADV_SDL3_VERSION}.tar.gz
  URL_HASH SHA256=f3bb06f55bed9557227a700af2ea62e49497ebdf58037732a731a5fb3ef8581f
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
# Not needed by the game; keeps the build lean.
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
# SDL uses GNU extensions (EM_ASM etc.); our own targets stay strict C17.
set(_cadv_saved_ext ${CMAKE_C_EXTENSIONS})
set(CMAKE_C_EXTENSIONS ON)
FetchContent_MakeAvailable(SDL3)
set(CMAKE_C_EXTENSIONS ${_cadv_saved_ext})
if(NOT TARGET SDL3::SDL3)
  add_library(SDL3::SDL3 ALIAS SDL3-static)
endif()
