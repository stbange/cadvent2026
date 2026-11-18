# SPDX-License-Identifier: MIT
# Usage: cmake -DCADVENT=... -DPPMPROBE=... -DOUT=... -P smoke_triangle.cmake
file(REMOVE "${OUT}")
execute_process(COMMAND "${CADVENT}" --frames 3 --width 320 --height 180 --screenshot "${OUT}" --log-level debug
  RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "cadvent exited with ${rc}")
endif()
# centre of the triangle: fir green
execute_process(COMMAND "${PPMPROBE}" "${OUT}" 50 50 26 140 60 RESULT_VARIABLE rc1)
# top-left corner: night-blue clear colour (with explicit tolerance)
execute_process(COMMAND "${PPMPROBE}" "${OUT}" 2 2 16 20 48 3 RESULT_VARIABLE rc2)
# just outside the triangle's left edge: still clear colour
execute_process(COMMAND "${PPMPROBE}" "${OUT}" 10 80 16 20 48 RESULT_VARIABLE rc3)
# negative check: the centre is NOT the clear colour
execute_process(COMMAND "${PPMPROBE}" "${OUT}" 50 50 16 20 48 RESULT_VARIABLE rc4)
if(NOT rc1 EQUAL 0 OR NOT rc2 EQUAL 0 OR NOT rc3 EQUAL 0 OR rc4 EQUAL 0)
  message(FATAL_ERROR "screenshot probe failed: ${rc1} ${rc2} ${rc3} ${rc4}")
endif()
