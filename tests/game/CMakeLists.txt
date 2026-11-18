# SPDX-License-Identifier: MIT
# Headless game tests: run the real executable and inspect its output.
if(NOT TARGET cadvent OR NOT TARGET ppmprobe)
  return()
endif()

add_test(NAME game.cli_version COMMAND cadvent --version)
set_tests_properties(game.cli_version PROPERTIES LABELS game PASS_REGULAR_EXPRESSION "cAdvent 2026 [0-9]+\\.[0-9]+\\.[0-9]+")
add_test(NAME game.cli_help COMMAND cadvent --help)
set_tests_properties(game.cli_help PROPERTIES LABELS game PASS_REGULAR_EXPRESSION "usage: cadvent")
add_test(NAME game.cli_rejects_unknown_option COMMAND cadvent --bogus)
set_tests_properties(game.cli_rejects_unknown_option PROPERTIES LABELS game WILL_FAIL TRUE)

add_test(NAME tool.ppmprobe_usage COMMAND ppmprobe)
set_tests_properties(tool.ppmprobe_usage PROPERTIES LABELS game WILL_FAIL TRUE)
add_test(NAME tool.ppmprobe_missing_file COMMAND ppmprobe does-not-exist.ppm 50 50 0 0 0)
set_tests_properties(tool.ppmprobe_missing_file PROPERTIES LABELS game WILL_FAIL TRUE)

if(CADV_GFX_TESTS)
  # Renders 3 frames at 320x180, writes a screenshot and probes it:
  # centre = fir-green triangle, corner = night-blue clear colour.
  add_test(NAME game.smoke_triangle
    COMMAND ${CMAKE_COMMAND}
      -DCADVENT=$<TARGET_FILE:cadvent> -DPPMPROBE=$<TARGET_FILE:ppmprobe>
      -DOUT=${CMAKE_CURRENT_BINARY_DIR}/smoke_triangle.ppm
      -P ${CMAKE_CURRENT_SOURCE_DIR}/smoke_triangle.cmake)
  set_tests_properties(game.smoke_triangle PROPERTIES LABELS "game;gfx" TIMEOUT 60)

  add_test(NAME game.frames_without_screenshot COMMAND cadvent --frames 2 --width 64 --height 64)
  set_tests_properties(game.frames_without_screenshot PROPERTIES LABELS "game;gfx" PASS_REGULAR_EXPRESSION "shutdown after 2 frames" TIMEOUT 60)
  add_test(NAME game.screenshot_unwritable_path
    COMMAND cadvent --frames 1 --width 64 --height 64 --screenshot /nonexistent-dir/x.ppm)
  set_tests_properties(game.screenshot_unwritable_path PROPERTIES LABELS "game;gfx" WILL_FAIL TRUE TIMEOUT 60)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # SDL's offscreen driver + EGL renders with Mesa llvmpipe, no display needed.
    set_tests_properties(game.smoke_triangle game.screenshot_unwritable_path game.frames_without_screenshot
      PROPERTIES ENVIRONMENT "SDL_VIDEO_DRIVER=offscreen")
  endif()
endif()

if(CADV_SANITIZE)
  get_property(_game_tests DIRECTORY PROPERTY TESTS)
  set_property(TEST ${_game_tests} APPEND PROPERTY ENVIRONMENT
    "LSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/tests/sanitizers/lsan.supp:print_suppressions=0")
endif()
