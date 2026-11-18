# SPDX-License-Identifier: MIT
# Coverage and sanitizer instrumentation for first-party targets only.
if(CADV_COVERAGE)
  if(MSVC)
    message(FATAL_ERROR "CADV_COVERAGE requires GCC or Clang")
  endif()
  foreach(t IN LISTS CADV_FIRST_PARTY_TARGETS)
    target_compile_options(${t} PRIVATE --coverage -O0 -g)
    target_link_options(${t} PRIVATE --coverage)
  endforeach()
  find_program(GCOVR_EXE gcovr)
  if(GCOVR_EXE)
    add_custom_target(coverage
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND ${GCOVR_EXE} --config ${CMAKE_SOURCE_DIR}/gcovr.cfg
              --root ${CMAKE_SOURCE_DIR} --object-directory ${CMAKE_BINARY_DIR}
              --html-details ${CMAKE_BINARY_DIR}/coverage/index.html
              --xml ${CMAKE_BINARY_DIR}/coverage/coverage.xml
              --txt-summary
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Coverage gate (see gcovr.cfg)"
      VERBATIM)
  endif()
endif()

if(CADV_SANITIZE)
  if(MSVC)
    message(FATAL_ERROR "CADV_SANITIZE requires GCC or Clang")
  endif()
  set(_san -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  foreach(t IN LISTS CADV_FIRST_PARTY_TARGETS)
    target_compile_options(${t} PRIVATE ${_san})
    target_link_options(${t} PRIVATE ${_san})
  endforeach()
endif()
