# Run the external host generator without accepting stale or incomplete outputs.
# Check exit status, diagnostics and outputs; never rely on an old generated file.
foreach(required COCOR_EXECUTABLE COCOR_ROOT GRAMMAR)
  if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${required} for Coco/R generation")
  endif()
endforeach()
set(outputs "${GRAMMAR}p.c" "${GRAMMAR}s.c" "${GRAMMAR}p.h"
            "${GRAMMAR}s.h" "${GRAMMAR}c.h" "${GRAMMAR}e.h")
# Never reuse a previous successful generation after this invocation fails.
file(REMOVE ${outputs})
execute_process(
  COMMAND "${CMAKE_COMMAND}" -E env "CRFRAMES=${COCOR_ROOT}/frames"
          "${COCOR_EXECUTABLE}" "${GRAMMAR}.atg"
  RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
string(TOLOWER "${output}\n${error}" diagnostics)
if(NOT status EQUAL 0 OR diagnostics MATCHES "\\(e[0-9]+\\)|ll\\(1\\) error|not ll\\(1\\)")
  message(FATAL_ERROR "Coco/R failed (${status}):\n${output}\n${error}")
endif()
foreach(path IN LISTS outputs)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Coco/R did not generate ${path}:\n${output}\n${error}")
  endif()
endforeach()
message(STATUS "Coco/R ${GRAMMAR}: ${output}${error}")
