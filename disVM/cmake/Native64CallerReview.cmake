# Execute the selected guarded vmachine and its two semantic mutation controls.
# No owning library archive or sibling checkout is required. Native Linux AMD64
# and ARM64 run their own RX backend; cross builds require a separate target gate.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING
   AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND backend MATCHES "^(amd64|arm64)$")
  foreach(control normal enter unwind)
    set(caller_arguments)
    if(NOT control STREQUAL "normal")
      list(APPEND caller_arguments --control "${control}")
    endif()
    add_test(NAME disvm_native64_live_${control} COMMAND "${Python3_EXECUTABLE}"
      "${u}/utils/tests/native64-live-vm.py" ${caller_arguments})
    set_tests_properties(disvm_native64_live_${control} PROPERTIES TIMEOUT 180
      ENVIRONMENT "CC=${CMAKE_C_COMPILER};NATIVE64_ARCH=${backend};NATIVE64_CFLAGS=-O3 -g -DNDEBUG -fno-strict-aliasing -fsanitize=undefined -fno-sanitize-recover=all;NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  endforeach()
endif()
