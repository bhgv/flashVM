# Experimental native-cell Dis runner; does not reinterpret legacy modules.
cmake_minimum_required(VERSION 3.10)
project(disvm_runtime VERSION 0.1.0 LANGUAGES C)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  message(FATAL_ERROR "Experimental runtime acceptance currently requires native Linux 64-bit")
endif()
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
find_package(PythonInterp 3 REQUIRED)
get_filename_component(root "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(DVM_FLASHVM_SOURCE_ROOT "${root}/flashVM" CACHE PATH "Shared FlashVM source")
set(LIMBO_BOOTSTRAP_CELL_BYTES 8 CACHE STRING "Native-cell experimental compiler" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "Build focused runtime checks separately" FORCE)
add_subdirectory("${root}/tools/limbo-bootstrap" bootstrap)
add_subdirectory("${root}/disVM/host" host)
set(u "${root}/disVM/upstream")
set(view "${CMAKE_CURRENT_BINARY_DIR}/native")
file(MAKE_DIRECTORY "${view}")
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${root}/disVM/host/prepare_native_registers.py"
  --source "${u}/libinterp" --output "${view}" RESULT_VARIABLE prepared)
if(NOT prepared EQUAL 0)
  message(FATAL_ERROR "Native source preparation failed")
endif()
file(READ "${u}/include/isa.h" isa)
string(REPLACE "IBY2WD\t= 4" "IBY2WD\t= 8" isa "${isa}")
string(REPLACE "XMAGIC\t= 819248" "XMAGIC\t= 819264" isa "${isa}")
string(REPLACE "SMAGIC\t= 923426" "SMAGIC\t= 923442" isa "${isa}")
file(WRITE "${view}/isa.h" "${isa}")
file(READ "${u}/include/interp.h" native_interp)
string(REPLACE "\tint\tnsend;" "\tWORD\tnsend;" native_interp "${native_interp}")
string(REPLACE "\tint\tnrecv;" "\tWORD\tnrecv;" native_interp "${native_interp}")
file(WRITE "${view}/interp.h" "${native_interp}\n#include \"disvm/native_context.h\"\nextern REG *dvm_native_registers(void);\n#define DVM_R (*dvm_native_registers())\nextern int dvm_runtime_leaf(Jit *, int, REG *);\nextern unsigned long dvm_runtime_instructions;\n")
# Source-view regeneration must never silently drop reviewed local adaptations.
file(REMOVE "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c")

add_custom_command(OUTPUT "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c"
 COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py"
   --compiler "$<TARGET_FILE:limbo-bootstrap>" --sys "${u}/module/sys.m" --source "${u}/libinterp" --output "${view}"
 DEPENDS limbo-bootstrap prepare.py "${u}/module/sys.m" VERBATIM)
file(GLOB runtime_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h" "${u}/include/isa.h" "${u}/include/interp.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runtime_inputs}
  "${root}/disVM/host/prepare_native_registers.py")
set(core alt conv dec disc gc heap link load raise readmod runt stack string validstk xec comp-amd64)
set(sources)
foreach(unit IN LISTS core)
 list(APPEND sources "${view}/${unit}.c")
endforeach()
add_custom_target(disvm_runtime_sources DEPENDS "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c")
add_executable(disvm main.c "${view}/unsupported.c" ${sources} "${root}/disVM/host/native_registers.c")
add_dependencies(disvm disvm_runtime_sources)
target_compile_definitions(disvm PRIVATE DVM_NATIVE_REGISTERS=1)
target_compile_options(disvm PRIVATE -fno-strict-aliasing)
target_include_directories(disvm PRIVATE "${view}" "${root}/disVM/host/include" "${u}/Linux/arm/include"
 "${u}/include" "${u}/emu/port" "${root}/disVM/include")
target_link_libraries(disvm PRIVATE disvm_flashvm_host limbo_bootstrap_lib9 limbo_bootstrap_support limbo_bootstrap_lib9 m)



# Native-cell IDE modules live beside their matching runner, never in the legacy package.
set(includes -I "${u}/module" -I "${root}/ide/dis" -I "${root}/ide/dis/bridge"
  -I "${root}/ide/dis/examples/workspace" -I "${root}/disVM/module")
set(module_names idecore ideworkspace workspace-example ide-console console-check uibridge ideinput idebridge idecomplete idebuiltins)
set(module_sources "${root}/ide/dis/idecore.b" "${root}/ide/dis/bridge/ideworkspace.b"
  "${root}/ide/dis/examples/workspace/main.b" "${root}/ide/dis/examples/native-console/main.b"
  "${root}/disVM/examples/console-check/main.b" "${root}/disVM/limbo/uibridge.b"
  "${root}/ide/dis/bridge/ideinput.b" "${root}/ide/dis/bridge/idebridge.b"
  "${root}/ide/dis/bridge/idecomplete.b" "${root}/ide/dis/bridge/idebuiltins.b")
file(GLOB module_headers "${root}/ide/dis/*.m" "${root}/ide/dis/bridge/*.m"
  "${root}/ide/dis/examples/workspace/*.m" "${root}/disVM/module/*.m")
list(LENGTH module_names module_count)
math(EXPR module_last "${module_count}-1")
set(products)
foreach(index RANGE ${module_last})
 list(GET module_names ${index} name)
 list(GET module_sources ${index} source)
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${name}.dis" "${CMAKE_CURRENT_BINARY_DIR}/${name}.sbl"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/${name}.dis" "${source}"
  DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
 list(APPEND products "${CMAKE_CURRENT_BINARY_DIR}/${name}.dis")
endforeach()
add_custom_target(disvm_native_modules ALL DEPENDS ${products})
option(DVM_RUNTIME_TESTS "Run real native Dis IDE service regressions" ON)
if(DVM_RUNTIME_TESTS)
 enable_testing()
 foreach(test idecore limits coordinates find history replaceall utf8span ideworkspace idebridge idecomplete idebuiltins)
  set(source "${root}/ide/dis/tests/${test}_test.b")
  set(binary "${CMAKE_CURRENT_BINARY_DIR}/${test}-test.dis")
  add_custom_command(OUTPUT "${binary}"
   COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${binary}" "${source}"
   DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
  add_custom_target(disvm_native_${test} ALL DEPENDS "${binary}")
  foreach(mode jit interpreter)
   add_test(NAME disvm_application_${test}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>" "${binary}" "${mode}")
   set_tests_properties(disvm_application_${test}_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
 endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  # Generate a classic module using the separate legacy compiler in the workspace.
  set(DVM_LEGACY_IMAGE "" CACHE FILEPATH "Optional classic Dis image for ABI rejection test")
  if(DVM_LEGACY_IMAGE)
    add_test(NAME disvm_application_input_contract COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_inputs.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${DVM_LEGACY_IMAGE}")
    set_tests_properties(disvm_application_input_contract PROPERTIES TIMEOUT 30)
  endif()
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_file_size_test test_file_size.c)
  target_link_libraries(disvm_file_size_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_file_extent_preserves_position COMMAND disvm_file_size_test
    "${CMAKE_CURRENT_BINARY_DIR}/file-size-test.bin")
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_module_byte_descriptors COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_byte_descriptor.py" "${u}")
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
  set_tests_properties(disvm_module_byte_descriptors PROPERTIES TIMEOUT 30
    ENVIRONMENT "CC=${CMAKE_C_COMPILER};TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_native_generation_idempotent COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_generation.py" "$<TARGET_FILE:limbo-bootstrap>" "${u}/libinterp")
  set_tests_properties(disvm_native_generation_idempotent PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()
