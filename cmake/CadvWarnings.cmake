# SPDX-License-Identifier: MIT
# Strict warnings for first-party code.
function(cadv_strict_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /permissive- /D_CRT_SECURE_NO_WARNINGS)
    if(CADV_WERROR)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes -Wmissing-prototypes
      -Wpointer-arith -Wcast-qual -Wformat=2 -ffp-contract=off)
    if(CADV_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# Relaxed warnings for test code (Unity test functions have no prototypes).
function(cadv_test_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -ffp-contract=off)
    if(CADV_WERROR)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()
