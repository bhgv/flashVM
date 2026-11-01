# Explicit owning-target review; default builds require no cross toolchain.
set(DVM_RV64_CC "" CACHE STRING "RV64 cross compiler command for owning dispatch review")
set(DVM_RV64_RUNNER "" CACHE STRING "RV64 target runner command for owning dispatch review")
if(DVM_RV64_CC OR DVM_RV64_RUNNER)
  if(NOT DVM_RV64_CC OR NOT DVM_RV64_RUNNER)
    message(FATAL_ERROR "Set both DVM_RV64_CC and DVM_RV64_RUNNER")
  endif()
  add_test(NAME disvm_riscv_dispatch_target COMMAND "${Python3_EXECUTABLE}"
    "${u}/libinterp/tests/riscv-dispatch.py" --cc "${DVM_RV64_CC}"
    --runner "${DVM_RV64_RUNNER}")
  set_tests_properties(disvm_riscv_dispatch_target PROPERTIES TIMEOUT 240
    ENVIRONMENT "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()
