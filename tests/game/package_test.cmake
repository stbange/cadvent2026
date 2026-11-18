# SPDX-License-Identifier: MIT
# platform_builds__run_from_extracted_archive: package the build, extract the
# archive somewhere else and run Day 1 from there (assets must be found next
# to the executable, not in the source tree or the working directory).
set(root "${WORK}/pkg")
file(REMOVE_RECURSE "${root}")
file(MAKE_DIRECTORY "${root}/extract" "${root}/elsewhere")
execute_process(COMMAND "${CMAKE_COMMAND}" -DEXE=${CADVENT} -DSRC=${SRC} -DOUT=${root}/cadvent
                        -DARCHIVE=${root}/cadvent.tar.gz -P ${SRC}/cmake/CadvPackage.cmake
                RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "packaging failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E tar xzf "${root}/cadvent.tar.gz" WORKING_DIRECTORY "${root}/extract"
                RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "extracting failed")
endif()
get_filename_component(exe_name "${CADVENT}" NAME)
set(ENV{CADV_STRICT_ASSETS} 1)
execute_process(COMMAND "${root}/extract/cadvent/${exe_name}" --headless --date 2026-12-01 --portal 1
                        --save "${root}/elsewhere/p.sav" --script "${SCRIPT}"
                WORKING_DIRECTORY "${root}/elsewhere" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "the extracted game did not run (exit ${rc})")
endif()
