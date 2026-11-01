# Optional real compiler input; neither an implicit sibling build nor a deployed
# Python requirement. This gate remains separate from default file execution.
set(DVM_NATIVE64_LIMBO_EXECUTABLE "" CACHE FILEPATH "Source-built Limbo for scalar Dis64/field interoperability tests")
if(DVM_NATIVE64_LIMBO_EXECUTABLE)
  if(NOT EXISTS "${DVM_NATIVE64_LIMBO_EXECUTABLE}" OR IS_DIRECTORY "${DVM_NATIVE64_LIMBO_EXECUTABLE}")
    message(FATAL_ERROR "DVM_NATIVE64_LIMBO_EXECUTABLE must name an existing executable")
  endif()
  add_test(NAME disvm_dis64_external_reader COMMAND "${Python3_EXECUTABLE}"
    "${u}/utils/tests/dis64-external-reader.py"
    --limbo "${DVM_NATIVE64_LIMBO_EXECUTABLE}" --reader "$<TARGET_FILE:dis64inspect>")
  set_tests_properties(disvm_dis64_external_reader PROPERTIES TIMEOUT 120
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING
     AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND backend MATCHES "^(amd64|arm64)$")
    foreach(control normal kind owner generation field)
      set(field_arguments)
      if(NOT control STREQUAL "normal")
        list(APPEND field_arguments --control "${control}")
      endif()
      add_test(NAME disvm_native64_field_${control} COMMAND "${Python3_EXECUTABLE}"
        "${u}/utils/tests/native64-field.py" --limbo "${DVM_NATIVE64_LIMBO_EXECUTABLE}" ${field_arguments})
      set_tests_properties(disvm_native64_field_${control} PROPERTIES TIMEOUT 180
        ENVIRONMENT "CC=${CMAKE_C_COMPILER};NATIVE64_ARCH=${backend};NATIVE64_CFLAGS=-O3 -g -DNDEBUG -fno-strict-aliasing -fsanitize=undefined -fno-sanitize-recover=all;NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp;TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
    endforeach()
  endif()
endif()

add_test(NAME disvm_dis64_location_read COMMAND "${Python3_EXECUTABLE}"
  "${u}/utils/tests/dis64-location-read.py")
if(DVM_NATIVE64_LIMBO_EXECUTABLE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_test(NAME disvm_dis64_locations COMMAND "${Python3_EXECUTABLE}"
    "${u}/utils/tests/dis64-locations.py" --limbo "${DVM_NATIVE64_LIMBO_EXECUTABLE}"
    --root "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  set_tests_properties(disvm_dis64_locations PROPERTIES TIMEOUT 120
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp")
endif()
