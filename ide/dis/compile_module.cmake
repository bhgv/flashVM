# Fail closed on compiler failure/missing debug outputs; build products are not published artifacts.
foreach(required LIMBO SOURCE INCLUDE_DIR OUTPUT SYMBOLS)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required}")
  endif()
endforeach()
# Remove previous products before invoking the compiler so stale files cannot count as success.
file(REMOVE "${OUTPUT}" "${SYMBOLS}")
execute_process(COMMAND "${LIMBO}" -g -I "${INCLUDE_DIR}" -o "${OUTPUT}" "${SOURCE}"
  RESULT_VARIABLE status OUTPUT_VARIABLE compiler_stdout ERROR_VARIABLE compiler_stderr
  TIMEOUT 60)
if(NOT "${status}" STREQUAL "0")
  file(REMOVE "${OUTPUT}" "${SYMBOLS}")
  message(FATAL_ERROR "Limbo compilation failed: ${status}\n${compiler_stdout}\n${compiler_stderr}")
endif()
foreach(product "${OUTPUT}" "${SYMBOLS}")
  if(NOT EXISTS "${product}" OR IS_DIRECTORY "${product}")
    file(REMOVE "${OUTPUT}" "${SYMBOLS}")
    message(FATAL_ERROR "Limbo did not produce both module and debug symbols")
  endif()
  # A bounded byte read also works on CMake 3.10, before file(SIZE).
  file(READ "${product}" product_prefix LIMIT 1 HEX)
  if("${product_prefix}" STREQUAL "")
    file(REMOVE "${OUTPUT}" "${SYMBOLS}")
    message(FATAL_ERROR "Empty Limbo build product")
  endif()
endforeach()
# XMAGIC=819248 uses discon's four-byte encoding c0 0c 80 30.
# Only the normal unsigned compiler product is admitted; this is not a Dis verifier.
# Read a fixed prefix, so admission never scans an arbitrarily large binary file.
file(READ "${OUTPUT}" module_magic LIMIT 4 HEX)
if(NOT "${module_magic}" STREQUAL "c00c8030")
  file(REMOVE "${OUTPUT}" "${SYMBOLS}")
  message(FATAL_ERROR "Unexpected IDE module signature; expected unsigned Dis XMAGIC")
endif()
# Match the first two physical lines byte-for-byte, including their terminators.
# file(STRINGS) skips empty/non-text bytes and cannot validate this wire prefix.
# Each line may use LF or CRLF, matching the existing Coco/R .sbl reader contract.
file(READ "${SYMBOLS}" symbol_header LIMIT 25 HEX)
if(NOT "${symbol_header}" MATCHES "^6c696d626f202e73626c20322e31(0d)?0a496465636f7265(0d)?0a")
  file(REMOVE "${OUTPUT}" "${SYMBOLS}")
  message(FATAL_ERROR "Unexpected IDE symbol header; expected limbo .sbl 2.1 / Idecore")
endif()
