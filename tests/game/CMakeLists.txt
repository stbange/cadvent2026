# SPDX-License-Identifier: MIT
# Game tests: run the real executable. Scenario tests are headless (no GPU)
# and run everywhere; rendering tests need an OpenGL driver (Mesa llvmpipe in CI).
if(NOT TARGET cadvent OR NOT TARGET imgdiff)
  return()
endif()

set(GAME_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(FIX ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)

add_test(NAME game.cli_version COMMAND cadvent --version)
set_tests_properties(game.cli_version PROPERTIES LABELS game PASS_REGULAR_EXPRESSION "cAdvent 2026 [0-9]+\\.[0-9]+\\.[0-9]+")
add_test(NAME game.cli_help COMMAND cadvent --help)
set_tests_properties(game.cli_help PROPERTIES LABELS game PASS_REGULAR_EXPRESSION "usage: cadvent")
add_test(NAME game.cli_rejects_unknown_option COMMAND cadvent --bogus)
set_tests_properties(game.cli_rejects_unknown_option PROPERTIES LABELS game WILL_FAIL TRUE)
add_test(NAME game.script_missing_file COMMAND cadvent --headless --script does-not-exist.gts)
set_tests_properties(game.script_missing_file PROPERTIES LABELS game WILL_FAIL TRUE)
add_test(NAME game.headless_plain_run COMMAND cadvent --headless --frames 120 --save ${CMAKE_CURRENT_BINARY_DIR}/plain.sav)
set_tests_properties(game.headless_plain_run PROPERTIES LABELS game)

add_test(NAME tool.imgdiff_usage COMMAND imgdiff)
set_tests_properties(tool.imgdiff_usage PROPERTIES LABELS game WILL_FAIL TRUE)
add_test(NAME tool.imgdiff_missing COMMAND imgdiff a.ppm b.ppm)
set_tests_properties(tool.imgdiff_missing PROPERTIES LABELS game WILL_FAIL TRUE)
add_test(NAME tool.imgdiff_identical
  COMMAND imgdiff ${FIX}/tiny_a.ppm ${FIX}/tiny_a.ppm 0 0 ${CMAKE_CURRENT_BINARY_DIR}/tiny_diff.ppm)
set_tests_properties(tool.imgdiff_identical PROPERTIES LABELS game)
add_test(NAME tool.imgdiff_different COMMAND imgdiff ${FIX}/tiny_a.ppm ${FIX}/tiny_b.ppm)
set_tests_properties(tool.imgdiff_different PROPERTIES LABELS game WILL_FAIL TRUE)
add_test(NAME tool.imgdiff_size_mismatch COMMAND imgdiff ${FIX}/tiny_a.ppm ${PROJECT_SOURCE_DIR}/tests/golden/hall_spawn.ppm)
set_tests_properties(tool.imgdiff_size_mismatch PROPERTIES LABELS game WILL_FAIL TRUE)

# name script [fixture] [args...]
function(cadv_scenario name script fixture args)
  set(save ${CMAKE_CURRENT_BINARY_DIR}/${name}.sav)
  add_test(NAME game.${name}
    COMMAND ${CMAKE_COMMAND} -DCADVENT=$<TARGET_FILE:cadvent> -DSCRIPT=${GAME_DIR}/${script}
            -DSAVE=${save} -DFIXTURE=${fixture} "-DARGS=${args}" ${ARGN}
            -P ${GAME_DIR}/run_gts.cmake)
  set_tests_properties(game.${name} PROPERTIES LABELS "game;scenario" TIMEOUT 300)
endfunction()

cadv_scenario(hub_reach_portal_24 hub_reach_portal_24.gts "" "--date 2026-11-30")
cadv_scenario(teaser teaser.gts "" "--date 2026-11-18")
cadv_scenario(help help.gts "" "--unlock-all --portal 1")
add_test(NAME game.package_runs_from_archive
  COMMAND ${CMAKE_COMMAND} -DCADVENT=$<TARGET_FILE:cadvent> -DSRC=${PROJECT_SOURCE_DIR}
          -DWORK=${CMAKE_CURRENT_BINARY_DIR} -DSCRIPT=${GAME_DIR}/package_smoke.gts
          -P ${GAME_DIR}/package_test.cmake)
set_tests_properties(game.package_runs_from_archive PROPERTIES LABELS game)
cadv_scenario(hub_sealed_portal hub_sealed_portal.gts "" "--date 2026-12-04T23:59")
cadv_scenario(hub_still_thawing hub_still_thawing.gts "" "--date 2026-12-05")
cadv_scenario(dev_enter_and_win dev_enter_and_win.gts "" "--unlock-all")
cadv_scenario(p01_enter_and_win p01_enter_and_win.gts "" "--date 2026-12-01")
cadv_scenario(p02_enter_and_win p02_enter_and_win.gts "" "--date 2026-12-02")
cadv_scenario(p03_enter_and_win p03_enter_and_win.gts "" "--date 2026-12-03")
cadv_scenario(p04_enter_and_win p04_enter_and_win.gts "" "--date 2026-12-04")
cadv_scenario(stars_socketing stars_socketing.gts ${FIX}/two_stars.sav "--date 2026-12-10")
cadv_scenario(gate_before_eve gate_before_eve.gts ${FIX}/arch_full.sav "--date 2026-12-23")
cadv_scenario(gate_on_eve gate_on_eve.gts ${FIX}/arch_full.sav "--date 2026-12-24")
cadv_scenario(gate_missing gate_missing.gts ${FIX}/arch_missing.sav "--date 2026-12-24")
cadv_scenario(epilogue epilogue.gts ${FIX}/star24.sav "--date 2026-12-24")
cadv_scenario(checkpoint_resume checkpoint_run1.gts "" "--unlock-all --portal 5" -DSCRIPT2=${GAME_DIR}/checkpoint_run2.gts)
cadv_scenario(marks marks.gts "" "--unlock-all --portal 5")
cadv_scenario(isolation isolation.gts "" "--unlock-all --portal 5")

if(CADV_GFX_TESTS)
  function(cadv_golden name args)
    add_test(NAME game.golden_${name}
      COMMAND ${CMAKE_COMMAND} -DCADVENT=$<TARGET_FILE:cadvent> -DIMGDIFF=$<TARGET_FILE:imgdiff>
              -DOUT=${CMAKE_CURRENT_BINARY_DIR}/golden_${name}.ppm
              -DGOLDEN=${PROJECT_SOURCE_DIR}/tests/golden/${name}.ppm "-DARGS=${args}"
              -P ${GAME_DIR}/golden.cmake)
    set_tests_properties(game.golden_${name} PROPERTIES LABELS "game;gfx;golden" TIMEOUT 120)
    list(APPEND CADV_GFX_TEST_NAMES game.golden_${name})
    set(CADV_GFX_TEST_NAMES ${CADV_GFX_TEST_NAMES} PARENT_SCOPE)
  endfunction()
  cadv_golden(hall_spawn "--frames 12 --date 2026-12-05 --save ${CMAKE_CURRENT_BINARY_DIR}/g1.sav")
  cadv_golden(hall_dev "--frames 12 --unlock-all --save ${CMAKE_CURRENT_BINARY_DIR}/g2.sav")
  cadv_golden(test_range "--frames 12 --unlock-all --portal 5 --save ${CMAKE_CURRENT_BINARY_DIR}/g3.sav")

  add_test(NAME game.render_scripted_tour
    COMMAND cadvent --width 320 --height 180 --unlock-all --save ${CMAKE_CURRENT_BINARY_DIR}/tour.sav
            --script ${GAME_DIR}/tour.gts)
  set_tests_properties(game.render_scripted_tour PROPERTIES LABELS "game;gfx" TIMEOUT 300
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
  # Rendered runs of Day 1 and the hall's ending (cover the GPU paths).
  function(cadv_render_scenario name script fixture args)
    set(save ${CMAKE_CURRENT_BINARY_DIR}/${name}.sav)
    add_test(NAME game.${name}
      COMMAND ${CMAKE_COMMAND} -DCADVENT=$<TARGET_FILE:cadvent> -DSCRIPT=${GAME_DIR}/${script}
              -DSAVE=${save} -DFIXTURE=${fixture} "-DARGS=${args}" -DRENDER=1
              -P ${GAME_DIR}/run_gts.cmake)
    set_tests_properties(game.${name} PROPERTIES LABELS "game;gfx" TIMEOUT 600
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    list(APPEND CADV_GFX_TEST_NAMES game.${name})
    set(CADV_GFX_TEST_NAMES ${CADV_GFX_TEST_NAMES} PARENT_SCOPE)
  endfunction()
  cadv_render_scenario(render_p01_tour p01_tour.gts "" "--date 2026-12-01 --portal 1")
  cadv_render_scenario(render_p01_finale p01_finale.gts ${FIX}/p01_roof.sav "--date 2026-12-01 --portal 1")
  cadv_render_scenario(render_p02_tour p02_tour.gts "" "--date 2026-12-02 --portal 2")
  cadv_render_scenario(render_p02_night p02_night.gts ${FIX}/p02_night.sav "--date 2026-12-02 --portal 2")
  cadv_render_scenario(render_p03_tour p03_tour.gts "" "--date 2026-12-03 --portal 3")
  cadv_render_scenario(render_p03_push p03_push.gts ${FIX}/p03_base.sav "--date 2026-12-03 --portal 3")
  cadv_render_scenario(render_p04_tour p04_tour.gts "" "--unlock-all --portal 4")
  cadv_render_scenario(render_p04_chase p04_chase.gts ${FIX}/p04_crypt.sav "--date 2026-12-04 --portal 4")
  cadv_render_scenario(render_epilogue epilogue.gts ${FIX}/star24.sav "--date 2026-12-24")

  add_test(NAME game.screenshot_unwritable_path
    COMMAND cadvent --frames 1 --width 64 --height 64 --save ${CMAKE_CURRENT_BINARY_DIR}/u.sav --screenshot /nonexistent-dir/x.ppm)
  set_tests_properties(game.screenshot_unwritable_path PROPERTIES LABELS "game;gfx" WILL_FAIL TRUE TIMEOUT 60)
  set(gfx_tests ${CADV_GFX_TEST_NAMES} game.render_scripted_tour game.screenshot_unwritable_path)
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # SDL's offscreen driver + EGL renders with Mesa llvmpipe, no display needed.
    set_tests_properties(${gfx_tests} PROPERTIES ENVIRONMENT "SDL_VIDEO_DRIVER=offscreen;SDL_AUDIO_DRIVER=dummy")
  endif()
endif()

if(CADV_SANITIZE)
  get_property(_game_tests DIRECTORY PROPERTY TESTS)
  set_property(TEST ${_game_tests} APPEND PROPERTY ENVIRONMENT
    "LSAN_OPTIONS=suppressions=${PROJECT_SOURCE_DIR}/tests/sanitizers/lsan.supp:print_suppressions=0")
endif()
