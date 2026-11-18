# SPDX-License-Identifier: MIT
# Renders a frame and compares it with a reference image:
#   cmake -DCADVENT=... -DIMGDIFF=... -DOUT=... -DGOLDEN=... -DARGS="..." -P golden.cmake
# Set CADV_UPDATE_GOLDEN=1 in the environment to (re)write the reference.
file(REMOVE "${OUT}")
separate_arguments(extra UNIX_COMMAND "${ARGS}")
execute_process(COMMAND "${CADVENT}" --width 400 --height 225 --internal-height 225 --screenshot "${OUT}" ${extra}
  RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "render failed (exit ${rc})")
endif()
if("$ENV{CADV_UPDATE_GOLDEN}" STREQUAL "1" OR NOT EXISTS "${GOLDEN}")
  configure_file("${OUT}" "${GOLDEN}" COPYONLY)
  message(STATUS "golden image written: ${GOLDEN}")
  return()
endif()
execute_process(COMMAND "${IMGDIFF}" "${OUT}" "${GOLDEN}" 24 2.0 "${OUT}.diff.ppm" RESULT_VARIABLE rc2)
if(NOT rc2 EQUAL 0)
  message(FATAL_ERROR "golden frame mismatch: ${OUT} vs ${GOLDEN} (see ${OUT}.diff.ppm)")
endif()
