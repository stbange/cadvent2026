# SPDX-License-Identifier: MIT
# Runs a scenario script headlessly with an optional save fixture:
#   cmake -DCADVENT=... -DSCRIPT=... -DSAVE=... [-DFIXTURE=...] [-DARGS="..."] -P run_gts.cmake
file(REMOVE "${SAVE}")
if(FIXTURE)
  configure_file("${FIXTURE}" "${SAVE}" COPYONLY)
endif()
separate_arguments(extra UNIX_COMMAND "${ARGS}")
if(RENDER)
  set(mode --width 320 --height 180)
else()
  set(mode --headless)
endif()
execute_process(COMMAND "${CADVENT}" ${mode} --script "${SCRIPT}" --save "${SAVE}" ${extra}
  RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "scenario failed (exit ${rc}): ${SCRIPT}")
endif()
if(SCRIPT2)
  execute_process(COMMAND "${CADVENT}" --headless --script "${SCRIPT2}" --save "${SAVE}" ${extra}
    RESULT_VARIABLE rc2)
  if(NOT rc2 EQUAL 0)
    message(FATAL_ERROR "second scenario failed (exit ${rc2}): ${SCRIPT2}")
  endif()
endif()
