# SPDX-License-Identifier: MIT
# Builds a self-contained release folder and archive (no installer needed).
#   cmake -DEXE=<built executable> -DSRC=<source dir> -DOUT=<folder to create>
#         [-DARCHIVE=<file.tar.gz|file.zip>] -P cmake/CadvPackage.cmake
# The folder holds the executable next to assets/ (found via SDL_GetBasePath),
# the licences, the asset credits and the third-party notices.
foreach(var EXE SRC OUT)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "CadvPackage: -D${var}=... is required")
  endif()
endforeach()
if(NOT EXISTS "${EXE}")
  message(FATAL_ERROR "CadvPackage: executable not found: ${EXE}")
endif()
file(REMOVE_RECURSE "${OUT}")
file(MAKE_DIRECTORY "${OUT}")
file(COPY "${EXE}" DESTINATION "${OUT}")
file(COPY "${SRC}/assets" DESTINATION "${OUT}")
foreach(f LICENSE LICENSE-ASSETS.md README.md)
  file(COPY "${SRC}/${f}" DESTINATION "${OUT}")
endforeach()
file(COPY_FILE "${SRC}/third_party/README.md" "${OUT}/THIRD-PARTY-NOTICES.md")
if(DEFINED ARCHIVE)
  get_filename_component(parent "${OUT}" DIRECTORY)
  get_filename_component(name "${OUT}" NAME)
  if(ARCHIVE MATCHES "\\.zip$")
    set(fmt zip)
  else()
    set(fmt gnutar)
  endif()
  if(fmt STREQUAL "zip")
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar cf "${ARCHIVE}" --format=zip "${name}"
                    WORKING_DIRECTORY "${parent}" RESULT_VARIABLE rc)
  else()
    execute_process(COMMAND "${CMAKE_COMMAND}" -E tar czf "${ARCHIVE}" "${name}"
                    WORKING_DIRECTORY "${parent}" RESULT_VARIABLE rc)
  endif()
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "CadvPackage: archiving failed (${rc})")
  endif()
endif()
