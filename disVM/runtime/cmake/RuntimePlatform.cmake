# Select a real decoded-leaf backend for the declared native Linux userspace.
# system/processor/pointer_bytes are copied CMake facts; output names the caller
# variable. Unsupported OS, word size or CPU is a configuration error, never an
# implicit interpreter fallback or an instruction-set alias.
function(dvm_runtime_select_backend system processor pointer_bytes output)
  if(NOT system STREQUAL "Linux" OR NOT pointer_bytes EQUAL 8)
    message(FATAL_ERROR "DisVM runtime requires Linux with 64-bit userspace; got ${system}/${processor}, pointer bytes=${pointer_bytes}")
  endif()
  string(TOLOWER "${processor}" normalized)
  if(normalized MATCHES "^(x86_64|amd64)$")
    set(backend amd64)
  elseif(normalized MATCHES "^(aarch64|arm64)$")
    set(backend arm64)
  elseif(normalized MATCHES "^(riscv64|rv64)$")
    set(backend riscv64)
  else()
    message(FATAL_ERROR "DisVM runtime has no selected backend for ${processor}; supported Linux CPUs: x86_64/amd64 aarch64/arm64 and riscv64/rv64")
  endif()
  set(${output} "${backend}" PARENT_SCOPE)
endfunction()
