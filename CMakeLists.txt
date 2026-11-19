# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.25)
project(cadvent2026 VERSION 0.1.0 DESCRIPTION "cAdvent 2026 - first-person retro advent calendar" LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CADV_BUILD_GAME "Build the game executable (needs SDL3)" ON)
option(CADV_BUILD_TESTS "Build unit and game tests" ON)
option(CADV_GFX_TESTS "Register tests that need a GPU/display (smoke, golden frames)" ON)
option(CADV_COVERAGE "Instrument first-party code for coverage (GCC/Clang)" OFF)
option(CADV_SANITIZE "Build with AddressSanitizer + UBSan (GCC/Clang)" OFF)
option(CADV_WERROR "Treat warnings as errors" ON)
option(CADV_SYSTEM_SDL3 "Use an installed SDL3 instead of fetching it" OFF)

if(EMSCRIPTEN)
  set(CADV_GFX_TESTS OFF CACHE BOOL "" FORCE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CadvWarnings)

# --- first-party libraries --------------------------------------------------
# cadv_core: pure C, no SDL, no GL. Everything here is unit-tested.
add_library(cadv_core STATIC
  src/core/arena.c
  src/core/args.c
  src/core/calendar.c
  src/core/clock.c
  src/core/kvstore.c
  src/core/mathx.c
  src/core/hash.c
  src/core/image.c
  src/core/log.c
  src/core/rng.c
  src/audio/audio.c
  src/input/input.c
  src/hub/friends.c
  src/hub/hub.c
  src/hub/layout.c
  src/hub/progress.c
  src/kits/fps/combat.c
  src/kits/fps/map.c
  src/kits/fps/player.c
  src/kits/platformer/climber.c
  src/kits/platformer/tiles.c
  src/kits/rts/grid.c
  src/kits/rts/rts.c
  src/kits/rts/rts_ai.c
  src/kits/vehicle/flyer.c
  src/kits/vehicle/walker.c
  src/kits/vehicle/terrain.c
  src/portal/help.c
  src/portal/registry.c
  src/portals/p01_sleighed.c
  src/portals/p02_aurora_carpet.c
  src/portals/p03_frostfront.c
  src/portals/p04_ice_tomb.c
  src/portals/p05_hearthguard.c
  src/portals/p06_iron_yule.c
  src/portals/test_range.c
  src/portal/script.c
  src/portal/session.c
  src/sim/collide.c
  src/gfx/font.c
  src/gfx/gltf.c
  src/gfx/mesh.c
  src/gfx/palette.c
  src/gfx/procgen.c
  src/gfx/scene.c
  src/gfx/shader_src.c
  src/gfx/texdata.c
  src/gfx/view.c
  src/gfx/voxel.c)
target_include_directories(cadv_core PUBLIC src)
target_include_directories(cadv_core SYSTEM PRIVATE third_party/stb third_party/cgltf)
if(NOT MSVC)
  target_link_libraries(cadv_core PUBLIC m)
endif()
cadv_strict_warnings(cadv_core)

# cadv_gfx: OpenGL renderer + app frame logic. Needs a GL context at runtime,
# covered by the headless game tests.
if(NOT EMSCRIPTEN)
  add_library(glad STATIC third_party/glad/src/gl.c)
  target_include_directories(glad SYSTEM PUBLIC third_party/glad/include)
endif()

add_library(cadv_gfx STATIC src/gfx/gl_util.c src/gfx/renderer.c src/portals/test_range_gfx.c src/portals/p01_sleighed_gfx.c src/portals/p02_aurora_carpet_gfx.c src/portals/p03_frostfront_gfx.c src/portals/p04_ice_tomb_gfx.c src/portals/p05_hearthguard_gfx.c src/portals/p06_iron_yule_gfx.c src/portals/views.c src/hub/hub_gfx.c src/app/app.c)
target_link_libraries(cadv_gfx PUBLIC cadv_core)
if(NOT EMSCRIPTEN)
  target_link_libraries(cadv_gfx PUBLIC glad)
endif()
cadv_strict_warnings(cadv_gfx)

# --- game executable ------------------------------------------------------------
if(CADV_BUILD_GAME)
  include(CadvSDL3)
  add_executable(cadvent src/platform/sdl_main.c)
  target_link_libraries(cadvent PRIVATE cadv_gfx SDL3::SDL3)
  target_compile_definitions(cadvent PRIVATE CADV_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
  cadv_strict_warnings(cadvent)
  if(EMSCRIPTEN)
    set_target_properties(cadvent PROPERTIES SUFFIX ".html")
    target_link_options(cadvent PRIVATE
      "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html"
      "-sMIN_WEBGL_VERSION=2" "-sMAX_WEBGL_VERSION=2"
      "-sALLOW_MEMORY_GROWTH=1" "-sENVIRONMENT=web" "-lidbfs.js"
      "-sEXPORTED_RUNTIME_METHODS=['FS']"
      "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets/text@/assets/text"
      "--preload-file=${CMAKE_CURRENT_SOURCE_DIR}/assets/maps@/assets/maps")
    set_property(TARGET cadvent APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
    # GitHub Pages publishes docs/ from main: copy the build to docs/play/.
    #   cmake --build --preset web --target publish-web
    set(CADV_PLAY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/docs/play")
    add_custom_target(publish-web
      COMMAND ${CMAKE_COMMAND} -E make_directory "${CADV_PLAY_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:cadvent>/cadvent.html" "${CADV_PLAY_DIR}/index.html"
      COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE_DIR:cadvent>/cadvent.js" "$<TARGET_FILE_DIR:cadvent>/cadvent.wasm"
              "$<TARGET_FILE_DIR:cadvent>/cadvent.data" "${CADV_PLAY_DIR}/"
      COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_SOURCE_DIR}/docs/.nojekyll"
      DEPENDS cadvent
      COMMENT "Publishing the web build to docs/play/ (commit it to update GitHub Pages)")
  endif()
endif()

# --- tools -------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
  add_executable(imgdiff tools/imgdiff/imgdiff.c)
  target_link_libraries(imgdiff PRIVATE cadv_core)
  cadv_strict_warnings(imgdiff)
endif()

# --- instrumentation -----------------------------------------------------------------
set(CADV_FIRST_PARTY_TARGETS cadv_core cadv_gfx)
if(TARGET cadvent)
  list(APPEND CADV_FIRST_PARTY_TARGETS cadvent)
endif()
if(TARGET imgdiff)
  list(APPEND CADV_FIRST_PARTY_TARGETS imgdiff)
endif()
include(CadvInstrument)

# --- tests ---------------------------------------------------------------------------
if(CADV_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
