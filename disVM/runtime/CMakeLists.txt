# Experimental native-cell Dis runner; does not reinterpret legacy modules.
cmake_minimum_required(VERSION 3.10)
project(disvm_runtime VERSION 0.4.0 LANGUAGES C)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  message(FATAL_ERROR "Experimental runtime acceptance currently requires native Linux 64-bit")
endif()
option(DVM_RUNTIME_EDITOR "Build the Dis-controlled FlashVM graphical editor" OFF)
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
file(REMOVE "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" "${view}/idehostmod.h")

set(host_prepare_args)
set(host_prepare_products)
if(DVM_RUNTIME_EDITOR)
  set(host_prepare_args --idehost "${root}/ide/dis/bridge/idehost.m")
  set(host_prepare_products "${view}/idehostmod.h")
endif()
add_custom_command(OUTPUT "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" ${host_prepare_products}
 COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py"
   --compiler "$<TARGET_FILE:limbo-bootstrap>" --sys "${u}/module/sys.m" ${host_prepare_args} --source "${u}/libinterp" --output "${view}"
 DEPENDS limbo-bootstrap prepare.py "${root}/ide/dis/bridge/idehost.m" native_tables.c native_math_types.c "${u}/module/sys.m" "${u}/module/srv.m" "${u}/module/loader.m" "${u}/module/math.m" "${u}/emu/port/srv.c" VERBATIM)
file(GLOB runtime_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h" "${u}/include/isa.h" "${u}/include/interp.h")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runtime_inputs}
  "${root}/disVM/host/prepare_native_registers.py")
set(core alt conv dec disc gc heap link load loader math raise readmod runt stack string validstk xec comp-amd64)
set(sources)
foreach(unit IN LISTS core)
 list(APPEND sources "${view}/${unit}.c")
endforeach()
add_custom_target(disvm_runtime_sources DEPENDS "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c")
add_executable(disvm main.c srv_adapter.c "${u}/libmath/blas.c" "${u}/libmath/gemm.c" "${u}/libmath/pow10.c" "${view}/srv.c" "${view}/unsupported.c" ${sources} "${root}/disVM/host/native_registers.c")
add_dependencies(disvm disvm_runtime_sources)
target_compile_definitions(disvm PRIVATE DVM_NATIVE_REGISTERS=1)
target_compile_options(disvm PRIVATE -fno-strict-aliasing)
target_include_directories(disvm PRIVATE "${view}" "${root}/disVM/host/include" "${u}/Linux/arm/include"
 "${u}/include" "${u}/emu/port" "${root}/disVM/include")
target_link_libraries(disvm PRIVATE disvm_flashvm_host limbo_bootstrap_lib9 limbo_bootstrap_support limbo_bootstrap_lib9 m)



execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${root}/disVM/libraries/prepare.py"
  --source "${u}" --output "${view}/libraries" --cells 8 RESULT_VARIABLE library_prepared)
if(NOT library_prepared EQUAL 0)
  message(FATAL_ERROR "Native library source preparation failed")
endif()
file(GLOB library_sources "${u}/appl/lib/*.b")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${library_sources}
  "${root}/disVM/libraries/prepare.py")

# Native-cell IDE modules live beside their matching runner, never in the legacy package.
set(includes -I "${u}/module" -I "${root}/ide/dis" -I "${root}/ide/dis/bridge"
  -I "${root}/ide/dis/examples/workspace" -I "${root}/disVM/module")
set(module_names idecore ideworkspace workspace-example ide-console console-check uibridge ideinput idebridge idecomplete idebuiltins editor-console idefiles bufio string arg readdir debug dis styx styxservers libraries-check ideviewport idefileops idesearch)
set(module_sources "${root}/ide/dis/idecore.b" "${root}/ide/dis/bridge/ideworkspace.b"
  "${root}/ide/dis/examples/workspace/main.b" "${root}/ide/dis/examples/native-console/main.b"
  "${root}/disVM/examples/console-check/main.b" "${root}/disVM/limbo/uibridge.b"
  "${root}/ide/dis/bridge/ideinput.b" "${root}/ide/dis/bridge/idebridge.b"
  "${root}/ide/dis/bridge/idecomplete.b" "${root}/ide/dis/bridge/idebuiltins.b"
  "${root}/ide/dis/examples/editor-console/main.b"
  "${root}/ide/dis/bridge/idefiles.b"
  "${view}/libraries/bufio.b" "${view}/libraries/string.b" "${view}/libraries/arg.b"
  "${view}/libraries/readdir.b" "${view}/libraries/debug.b" "${view}/libraries/dis.b"
  "${view}/libraries/styx.b" "${view}/libraries/styxservers.b"
  "${root}/disVM/examples/standard-modules/main.b"
  "${root}/ide/dis/bridge/ideviewport.b"
  "${root}/ide/dis/bridge/idefileops.b" "${root}/ide/dis/bridge/idesearch.b")
file(GLOB module_headers "${root}/ide/dis/*.m" "${root}/ide/dis/bridge/*.m"
  "${root}/ide/dis/examples/workspace/*.m" "${root}/disVM/module/*.m" "${u}/module/*.m")
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
 foreach(test idecore limits coordinates find history replaceall utf8span ideworkspace idebridge idecomplete idebuiltins navigation viewport uicodec words indent decimalappend)
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
   if(test STREQUAL "replaceall")
     # This million-character matrix is CPU-heavy; serialize it without weakening its timeout.
     set_tests_properties(disvm_application_${test}_${mode} PROPERTIES RUN_SERIAL TRUE)
   endif()
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

if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_editor_console_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_editor.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/editor-console.dis" "${mode}")
    set_tests_properties(disvm_editor_console_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_stdio_test test_stdio.c)
  target_link_libraries(disvm_stdio_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_standard_streams COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_stdio.py" "$<TARGET_FILE:disvm_stdio_test>")
  set_tests_properties(disvm_shared_standard_streams PROPERTIES TIMEOUT 30)
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_file_seek_test test_file_seek.c)
  target_link_libraries(disvm_file_seek_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_file_positioning COMMAND disvm_file_seek_test
    "${CMAKE_CURRENT_BINARY_DIR}/file-seek-test.bin")
  foreach(mode jit interpreter)
    add_test(NAME disvm_file_windows_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_file_windows.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${root}/ide/dis/bridge" "${mode}")
    set_tests_properties(disvm_file_windows_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_compiler_big_constant_pool COMMAND "${PYTHON_EXECUTABLE}"
    "${root}/tools/limbo-bootstrap/tests/big_constants.py" "$<TARGET_FILE:limbo-bootstrap>")
  set_tests_properties(disvm_compiler_big_constant_pool PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_resolver_test test_resolver.c)
  target_link_libraries(disvm_resolver_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_resolver COMMAND disvm_resolver_test)
  foreach(mode jit interpreter)
    add_test(NAME disvm_standard_modules_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/libraries-check.dis" "${mode}")
    add_test(NAME disvm_compiler_binary64_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_float_compiler.py" "$<TARGET_FILE:limbo-bootstrap>"
      "$<TARGET_FILE:disvm>" "${u}/module" "${mode}")
    set_tests_properties(disvm_standard_modules_${mode} disvm_compiler_binary64_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_executable(disvm_nc64_tables_test test_native_tables.c)
  add_test(NAME disvm_nc64_table_layout_and_errors COMMAND disvm_nc64_tables_test)
endif()

# Explicit opt-in: core/console builds still need no fonts, rasterizer or window SDK.
option(DVM_EDITOR_SDL "Enable the real SDL2 editor window and native events" OFF)
if(DVM_RUNTIME_EDITOR)
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/fonts")
  configure_file("${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf" COPYONLY)
  configure_file("${root}/ide/assets/fonts/OFL.txt" "${CMAKE_CURRENT_BINARY_DIR}/fonts/OFL.txt" COPYONLY)
  enable_language(CXX)
  set(CMAKE_CXX_STANDARD 98)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(FVM_BUILD_TESTS OFF CACHE BOOL "Editor owns focused integration checks" FORCE)
  set(FVM_BUILD_EXAMPLES OFF CACHE BOOL "Build only the embedded FlashVM" FORCE)
  add_subdirectory("${DVM_FLASHVM_SOURCE_ROOT}" flashvm)
  get_target_property(runtime_sources disvm SOURCES)
  get_target_property(runtime_includes disvm INCLUDE_DIRECTORIES)
  add_executable(disvm-editor ${runtime_sources} editor_host.c editor_view.c
    "${root}/disVM/host/dis_ui_channel.c")
  add_dependencies(disvm-editor disvm_runtime_sources)
  target_include_directories(disvm-editor PRIVATE ${runtime_includes} "${DVM_FLASHVM_SOURCE_ROOT}/ports")
  target_compile_definitions(disvm-editor PRIVATE DVM_NATIVE_REGISTERS=1 DVM_RUNTIME_EDITOR=1)
  target_compile_options(disvm-editor PRIVATE -fno-strict-aliasing)
  target_link_libraries(disvm-editor PRIVATE disvm_flashvm_host limbo_bootstrap_lib9
    limbo_bootstrap_support limbo_bootstrap_lib9 flashvm m)
  if(DVM_EDITOR_SDL)
    find_package(SDL2 2.0.9 CONFIG REQUIRED)
    target_compile_definitions(disvm-editor PRIVATE DVM_EDITOR_SDL=1)
    target_link_libraries(disvm-editor PRIVATE SDL2::SDL2)
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
      -o "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis"
      "${root}/ide/dis/examples/graphical-editor/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/graphical-editor/main.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_graphical_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis")
endif()

set(DVM_EDITOR_TEST_FONT "" CACHE FILEPATH "Optional explicit font override for graphical editor checks")
if(DVM_RUNTIME_EDITOR AND NOT DVM_EDITOR_TEST_FONT)
  set(DVM_EDITOR_TEST_FONT "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf")
endif()
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_editor_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_editor_${mode} PROPERTIES TIMEOUT 120)
    if(DVM_EDITOR_SDL)
      add_test(NAME disvm_graphical_sdl_upload_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}" window)
      set_tests_properties(disvm_graphical_sdl_upload_${mode} PROPERTIES TIMEOUT 120
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_actions_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_actions.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_actions_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_location_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_location.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_location_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_default_font_zoom_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_zoom.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_default_font_zoom_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_caret_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_caret.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_caret_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()
