# Experimental native-cell Dis runner; does not reinterpret legacy modules.
cmake_minimum_required(VERSION 3.10)
project(disvm_runtime VERSION 0.36.0 LANGUAGES C)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8
    OR NOT CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
  message(FATAL_ERROR "Experimental runtime acceptance currently requires native Linux 64-bit")
endif()
option(DVM_RUNTIME_NATIVE_DEMO "Build the documented native module tutorial" OFF)
option(DVM_RUNTIME_COMPILER "Link our C99 compiler/linker library service" OFF)
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
add_subdirectory("${root}/common/owned" owned)
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
file(WRITE "${view}/interp.h" "${native_interp}\n#include \"disvm/native_context.h\"\nextern REG *dvm_native_registers(void);\n#define DVM_R (*dvm_native_registers())\nextern int dvm_runtime_leaf(Jit *, int, REG *);\nextern int dvm_runtime_fast(int, int (*)(REG *), REG *);\nextern unsigned long dvm_runtime_instructions;\nextern unsigned long dvm_runtime_instruction_limit;\nextern void dvm_runtime_step(void);\n")
# Source-view regeneration must never silently drop reviewed local adaptations.
file(REMOVE "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" "${view}/idehostmod.h")

set(host_prepare_args)
set(host_prepare_products)
if(DVM_RUNTIME_COMPILER)
  add_subdirectory("${root}/disVM/compiler" compiler)
  list(APPEND host_prepare_args --fircompiler "${root}/disVM/module/fircompiler.m")
  list(APPEND host_prepare_products "${view}/fircompilermod.h")
endif()
if(DVM_RUNTIME_EDITOR)
  list(APPEND host_prepare_args --idehost "${root}/ide/dis/bridge/idehost.m")
  list(APPEND host_prepare_products "${view}/idehostmod.h")
endif()
if(DVM_RUNTIME_NATIVE_DEMO)
  set(native_demo_root "${root}/disVM/examples/native-module")
  set(DVM_LIMBO_BINDINGS "" CACHE FILEPATH "Our Coco/R Limbo C-interface generator")
  if(NOT EXISTS "${DVM_LIMBO_BINDINGS}")
    message(FATAL_ERROR "Native tutorial requires DVM_LIMBO_BINDINGS")
  endif()
  list(APPEND host_prepare_args --native-demo "${native_demo_root}/nativedemo.m" --native-bindings "${DVM_LIMBO_BINDINGS}")
  list(APPEND host_prepare_products "${view}/nativedemo_native.h")
endif()
file(GLOB runtime_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h" "${u}/include/isa.h" "${u}/include/interp.h" "${u}/include/vmfetch.h")
add_custom_command(OUTPUT "${view}/exception.c" "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" "${view}/bigtablemod.h" ${host_prepare_products}
 COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py"
   --compiler "$<TARGET_FILE:limbo-bootstrap>" --sys "${u}/module/sys.m" ${host_prepare_args} --source "${u}/libinterp" --output "${view}"
 DEPENDS limbo-bootstrap prepare.py "${root}/disVM/examples/native-module/nativedemo.m" ${DVM_LIMBO_BINDINGS} "${root}/disVM/module/bigtable.m" ${runtime_inputs} "${root}/ide/dis/bridge/idehost.m" "${root}/disVM/module/fircompiler.m" native_tables.c relative_sequence.c prepare_exception.py exception_host.h table_leases.h "${u}/emu/port/exception.c" "${root}/common/table/relative.h" "${u}/module/sys.m" "${u}/module/srv.m" "${u}/module/loader.m" "${u}/module/math.m" "${u}/emu/port/srv.c" VERBATIM)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runtime_inputs}
  "${root}/disVM/host/prepare_native_registers.py")
set(core alt conv dec disc exception gc heap link load loader math raise readmod runt stack string validstk xec comp-amd64)
set(sources)
foreach(unit IN LISTS core)
 list(APPEND sources "${view}/${unit}.c")
endforeach()
add_custom_target(disvm_runtime_sources DEPENDS "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c")
add_executable(disvm main.c sys_capture.c filesystem_sys.c srv_adapter.c bigtable.c "${u}/libmath/blas.c" "${u}/libmath/gemm.c" "${u}/libmath/pow10.c" "${view}/srv.c" "${view}/unsupported.c" ${sources} "${root}/disVM/host/native_registers.c")
add_dependencies(disvm disvm_runtime_sources)
if(DVM_RUNTIME_COMPILER)
  target_sources(disvm PRIVATE compiler_native.c)
  target_compile_definitions(disvm PRIVATE DVM_RUNTIME_COMPILER=1)
  target_link_libraries(disvm PRIVATE disvm_compiler_engine)
endif()

if(DVM_RUNTIME_NATIVE_DEMO)
  target_sources(disvm PRIVATE "${native_demo_root}/native.c")
  target_compile_definitions(disvm PRIVATE DVM_RUNTIME_NATIVE_DEMO=1)
endif()
target_compile_definitions(disvm PRIVATE DVM_NATIVE_REGISTERS=1 DVM_RUNTIME_VERSION="${PROJECT_VERSION}")
target_compile_options(disvm PRIVATE -fno-strict-aliasing)
target_include_directories(disvm PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${root}/common/table" "${view}" "${root}/disVM/host/include" "${u}/Linux/arm/include"
 "${u}/include" "${u}/emu/port" "${root}/disVM/include")
target_link_libraries(disvm PRIVATE fvm_owned_table disvm_flashvm_host limbo_bootstrap_lib9 limbo_bootstrap_support limbo_bootstrap_lib9 m)



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
  -I "${root}/ide/dis/examples/workspace" -I "${root}/disVM/module" -I "${root}/disVM/broker")
set(module_names idecore ideworkspace idecompiler idebuildpanel ideartifact idedeskpane idedesktop workspace-example ide-console console-check uibridge ideinput idebridge idecomplete idebuiltins editor-console idefiles bufio string arg readdir debug dis styx styxservers libraries-check ideviewport idefileops idesearch decimal-append ideclipboard idefield field-demo brokerdiag ideprojection projection-check ideinspect idevalue idedocprojection regex regex-check idebrowser)
set(module_sources "${root}/ide/dis/idecore.b" "${root}/ide/dis/bridge/ideworkspace.b"
  "${root}/ide/dis/bridge/idecompiler.b" "${root}/ide/dis/bridge/idebuildpanel.b" "${root}/ide/dis/bridge/ideartifact.b" "${root}/ide/dis/bridge/idedeskpane.b" "${root}/ide/dis/bridge/idedesktop.b"
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
  "${root}/ide/dis/bridge/idefileops.b" "${root}/ide/dis/bridge/idesearch.b"
  "${root}/disVM/examples/decimal-append/main.b" "${root}/ide/dis/bridge/ideclipboard.b" "${root}/ide/dis/bridge/idefield.b"
  "${root}/ide/dis/examples/graphical-editor/field-demo.b" "${root}/disVM/broker/brokerdiag.b" "${root}/ide/dis/bridge/ideprojection.b"
  "${root}/ide/dis/examples/projection/main.b" "${root}/ide/dis/bridge/ideinspect.b" "${root}/ide/dis/bridge/idevalue.b" "${root}/ide/dis/bridge/idedocprojection.b"
  "${view}/libraries/regex.b" "${u}/appl/lib/tests/regex-unicode.b" "${root}/ide/dis/bridge/idebrowser.b")
file(GLOB module_headers "${root}/ide/dis/*.m" "${root}/ide/dis/bridge/*.m"
  "${root}/ide/dis/examples/workspace/*.m" "${root}/disVM/module/*.m" "${root}/disVM/broker/*.m" "${u}/module/*.m")
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
  get_target_property(sys_capture_includes disvm INCLUDE_DIRECTORIES)
  add_executable(disvm_sys_capture_test test_sys_capture.c sys_capture.c)
  add_dependencies(disvm_sys_capture_test disvm_runtime_sources)
  target_include_directories(disvm_sys_capture_test PRIVATE ${sys_capture_includes})
  target_compile_definitions(disvm_sys_capture_test PRIVATE DVM_NATIVE_REGISTERS=1)
  foreach(capture_case RANGE 0 18)
    add_test(NAME disvm_sys_capture_${capture_case} COMMAND disvm_sys_capture_test ${capture_case})
    set_tests_properties(disvm_sys_capture_${capture_case} PROPERTIES TIMEOUT 10)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
 enable_testing()
 foreach(test idecore limits coordinates find history replaceall utf8span ideworkspace idebridge idecomplete idebuiltins navigation viewport uicodec words indent decimalappend clipboard field stringslice values docprojection hybridtable tableguards tableleases)
  set(source "${root}/ide/dis/tests/${test}_test.b")
  if(test STREQUAL "docprojection")
    set(source "${root}/ide/dis/examples/document-projection/main.b")
  endif()
  if(test STREQUAL "tableleases")
    set(source "${root}/disVM/examples/table-leases/main.b")
  endif()
  if(test STREQUAL "tableguards")
    set(source "${root}/disVM/examples/table-guards/main.b")
  endif()
  if(test STREQUAL "hybridtable")
    set(source "${root}/disVM/examples/hybrid-table/main.b")
  endif()
  set(binary "${CMAKE_CURRENT_BINARY_DIR}/${test}-test.dis")
  add_custom_command(OUTPUT "${binary}"
   COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${binary}" "${source}"
   DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
  add_custom_target(disvm_native_${test} ALL DEPENDS "${binary}")
  # The unchanged million-character replace-all matrix takes about 55 s with
  # checked interpreter operands on this host; preserve its data and all oracles.
  set(application_budget 50)
  if(test STREQUAL "replaceall")
    set(application_budget 90)
  endif()
  math(EXPR application_outer_budget "${application_budget} + 10")
  foreach(mode jit interpreter)
   add_test(NAME disvm_application_${test}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>" "${binary}" "${mode}" "${application_budget}")
   set_tests_properties(disvm_application_${test}_${mode} PROPERTIES TIMEOUT ${application_outer_budget})
   if(test STREQUAL "tableleases")
     set(lease_mode --no-jit)
     if(mode STREQUAL "jit")
       set(lease_mode --jit)
     endif()
     add_test(NAME disvm_tableleases_gc_${mode} COMMAND "$<TARGET_FILE:disvm>"
       ${lease_mode} --quantum 1 --stats --max-instructions 20000 "${binary}")
     set_tests_properties(disvm_tableleases_gc_${mode} PROPERTIES TIMEOUT 15
       PASS_REGULAR_EXPRESSION "tables_live=0 keys_live=0")
   endif()
   if(test STREQUAL "replaceall")
     # This million-character matrix is CPU-heavy; preserve all functional checks.
     # RUN_SERIAL covers this CTest instance only: run the separate core/JIT
     # checkpoint before this runtime checkpoint to avoid cross-instance contention.
     set_tests_properties(disvm_application_${test}_${mode} PROPERTIES RUN_SERIAL TRUE)
   endif()
  endforeach()
 endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_bigtable_boundary COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_bigtable_boundary.py" "$<TARGET_FILE:disvm>"
    "$<TARGET_FILE:limbo-bootstrap>" "${root}/disVM/module")
  set_tests_properties(disvm_bigtable_boundary PROPERTIES TIMEOUT 30
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
  foreach(mode jit interpreter)
    add_test(NAME disvm_decimal_append_example_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/decimal-append.dis" "${mode}")
    set_tests_properties(disvm_decimal_append_example_${mode} PROPERTIES TIMEOUT 60)
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
    "${CMAKE_CURRENT_SOURCE_DIR}/test_generation.py" "$<TARGET_FILE:limbo-bootstrap>" "${u}/libinterp" "${view}")
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
  get_target_property(editor_flashvm_definitions flashvm COMPILE_DEFINITIONS)
  list(FIND editor_flashvm_definitions RENDER_JOBS editor_render_jobs_index)
  if(editor_render_jobs_index EQUAL -1)
    message(FATAL_ERROR "The graphical editor requires RENDER_JOBS in the selected FlashVM DEFS")
  endif()
  get_target_property(runtime_sources disvm SOURCES)
  get_target_property(runtime_includes disvm INCLUDE_DIRECTORIES)
  add_executable(disvm-editor ${runtime_sources} editor_host.c editor_view.c editor_raster.c
    "${root}/disVM/host/dis_ui_channel.c")
  if(DVM_RUNTIME_TESTS)
    add_executable(disvm_editor_raster_test test_editor_raster.c editor_raster.c)
    target_link_libraries(disvm_editor_raster_test PRIVATE flashvm)
    add_test(NAME disvm_editor_detached_raster COMMAND disvm_editor_raster_test)
    set_tests_properties(disvm_editor_detached_raster PROPERTIES TIMEOUT 30)
  endif()
  add_dependencies(disvm-editor disvm_runtime_sources)
  if(DVM_RUNTIME_COMPILER)
    target_compile_definitions(disvm-editor PRIVATE DVM_RUNTIME_COMPILER=1)
    target_link_libraries(disvm-editor PRIVATE disvm_compiler_engine)
  endif()

  if(DVM_RUNTIME_NATIVE_DEMO)
    target_compile_definitions(disvm-editor PRIVATE DVM_RUNTIME_NATIVE_DEMO=1)
  endif()
  set(DVM_AS_VIEW_COMPILER "" CACHE FILEPATH "Source-built Coco/R AS2 display-profile compiler")
  if(NOT EXISTS "${DVM_AS_VIEW_COMPILER}")
    message(FATAL_ERROR "Build compiler/as2/view with Coco/R, then supply DVM_AS_VIEW_COMPILER")
  endif()
  set(DVM_FIR_COMPILER "" CACHE FILEPATH "FIR compiler CLI")
  set(DVM_FIR_OPTIMIZER "" CACHE FILEPATH "FIR peephole optimizer CLI")
  set(DVM_SWF_LINKER "" CACHE FILEPATH "SWL linker CLI")
  foreach(tool DVM_FIR_COMPILER DVM_FIR_OPTIMIZER DVM_SWF_LINKER)
    if(NOT EXISTS "${${tool}}")
      message(FATAL_ERROR "Supply ${tool} for AS -> FIR -> SWO -> SWF")
    endif()
  endforeach()
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp")
  add_custom_command(OUTPUT "${view}/ide_view_swf.h"
    BYPRODUCTS "${view}/ide-view.fir" "${view}/ide-view.opt.fir" "${view}/ide-view.swo" "${view}/ide-view.swf"
    COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/ide/as/editor-view.as" "${view}/ide-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" check "${view}/ide-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_OPTIMIZER}" "${view}/ide-view.fir" "${view}/ide-view.opt.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" object "${view}/ide-view.opt.fir" "${view}/ide-view.swo"
    COMMAND "${DVM_SWF_LINKER}" link "${view}/ide-view.swf" "${view}/ide-view.swo"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_view.py"
      "${view}/ide-view.swf" "${view}/ide_view_swf.h"
    DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}"
      "${DVM_SWF_LINKER}" "${root}/ide/as/editor-view.as" embed_view.py VERBATIM)
  target_sources(disvm-editor PRIVATE "${view}/ide_view_swf.h")
  add_custom_command(OUTPUT "${view}/desktop_view_swf.h"
    BYPRODUCTS "${view}/desktop-view.fir" "${view}/desktop-view.opt.fir" "${view}/desktop-view.swo" "${view}/desktop-view.swf"
    COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/ide/as/desktop-view.as" "${view}/desktop-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" check "${view}/desktop-view.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_OPTIMIZER}" "${view}/desktop-view.fir" "${view}/desktop-view.opt.fir"
    COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp"
      "${DVM_FIR_COMPILER}" object "${view}/desktop-view.opt.fir" "${view}/desktop-view.swo"
    COMMAND "${DVM_SWF_LINKER}" link "${view}/desktop-view.swf" "${view}/desktop-view.swo"
    COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_view.py"
      "${view}/desktop-view.swf" "${view}/desktop_view_swf.h"
    DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}"
      "${DVM_SWF_LINKER}" "${root}/ide/as/desktop-view.as" embed_view.py VERBATIM)
  target_sources(disvm-editor PRIVATE "${view}/desktop_view_swf.h")
  target_sources(disvm-editor PRIVATE desktop_view.c)
  if(DVM_RUNTIME_TESTS)
    add_executable(disvm_as_view_test test_as_view.c "${view}/ide_view_swf.h")
    target_include_directories(disvm_as_view_test PRIVATE "${view}")
    target_link_libraries(disvm_as_view_test PRIVATE flashvm)
    add_test(NAME disvm_as_view_execution COMMAND disvm_as_view_test)
  endif()
  target_include_directories(disvm-editor PRIVATE ${runtime_includes} "${DVM_FLASHVM_SOURCE_ROOT}/ports")
  target_compile_definitions(disvm-editor PRIVATE DVM_NATIVE_REGISTERS=1 DVM_RUNTIME_EDITOR=1 DVM_RUNTIME_VERSION="${PROJECT_VERSION}")
  target_compile_options(disvm-editor PRIVATE -fno-strict-aliasing)
  target_link_libraries(disvm-editor PRIVATE fvm_owned_table disvm_flashvm_host limbo_bootstrap_lib9
    limbo_bootstrap_support limbo_bootstrap_lib9 flashvm m)
  if(DVM_EDITOR_SDL)
    find_package(SDL2 2.0.9 CONFIG REQUIRED)
    target_compile_definitions(disvm-editor PRIVATE DVM_EDITOR_SDL=1)
    target_sources(disvm-editor PRIVATE editor_window_events.c)
    if(DVM_RUNTIME_TESTS)
      add_executable(disvm_window_events_test test_window_events.c editor_window_events.c)
      target_include_directories(disvm_window_events_test PRIVATE "${root}/disVM/include" "${DVM_FLASHVM_SOURCE_ROOT}/include")
      target_link_libraries(disvm_window_events_test PRIVATE SDL2::SDL2)
      add_test(NAME disvm_sdl_window_lifecycle COMMAND disvm_window_events_test)
      set_tests_properties(disvm_sdl_window_lifecycle PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    endif()
    target_link_libraries(disvm-editor PRIVATE SDL2::SDL2)
  endif()
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
      -o "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis"
      "${root}/ide/dis/examples/graphical-editor/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/graphical-editor/main.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_graphical_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis")
  # Ship an explicit IDE application entry; the channel controller remains reusable.
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ide.dis" "${CMAKE_CURRENT_BINARY_DIR}/ide.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -I "${root}/ide/dis/app"
      -o "${CMAKE_CURRENT_BINARY_DIR}/ide.dis" "${root}/ide/dis/app/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/app/main.b" "${root}/ide/dis/app/ide.m"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" ${module_headers} VERBATIM)
  add_custom_target(disvm_ide_application ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ide.dis")

endif()

set(DVM_EDITOR_TEST_FONT "" CACHE FILEPATH "Optional explicit font override for graphical editor checks")
if(DVM_RUNTIME_EDITOR AND NOT DVM_EDITOR_TEST_FONT)
  set(DVM_EDITOR_TEST_FONT "${CMAKE_CURRENT_BINARY_DIR}/fonts/NotoSansMono-Regular.ttf")
endif()
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_editor_async_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_async_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_editor_async_${mode} PROPERTIES TIMEOUT 60)
    add_test(NAME disvm_graphical_editor_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_graphical_editor_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_ide_application_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_editor.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_ide_application_${mode} PROPERTIES TIMEOUT 120)

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
    add_test(NAME disvm_graphical_fields_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_fields.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_fields_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_field_clipboard_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_field_clipboard.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_field_clipboard_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_resize_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_resize.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_resize_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_graphical_clipboard_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_clipboard.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}")
    set_tests_properties(disvm_graphical_clipboard_${mode} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE)
    if(DVM_EDITOR_SDL)
      add_test(NAME disvm_graphical_field_clipboard_sdl_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_field_clipboard.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}" sdl)
      set_tests_properties(disvm_graphical_field_clipboard_sdl_${mode} PROPERTIES TIMEOUT 120
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
      add_test(NAME disvm_graphical_clipboard_sdl_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_clipboard.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}" sdl)
      set_tests_properties(disvm_graphical_clipboard_sdl_${mode} PROPERTIES TIMEOUT 120 RUN_SERIAL TRUE
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
      add_test(NAME disvm_graphical_resize_sdl_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_resize.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" DEFAULT "${mode}" window)
      set_tests_properties(disvm_graphical_resize_sdl_${mode} PROPERTIES TIMEOUT 120
        ENVIRONMENT "SDL_VIDEODRIVER=dummy")
    endif()
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  # The long-line editor uses this public path transaction; reuse its actual allocation oracle under UBSan.
  add_executable(disvm_editor_path_transaction "${root}/flashVM/tests/path_append_transaction.cpp")
  target_link_libraries(disvm_editor_path_transaction PRIVATE flashvm)
  add_test(NAME disvm_editor_path_transaction COMMAND disvm_editor_path_transaction)
  set_tests_properties(disvm_editor_path_transaction PROPERTIES TIMEOUT 30)
endif()

# Optional cross-project acceptance never makes the standalone runtime depend on the FIR build.
set(DVM_FIR_COMPILER "" CACHE FILEPATH "Our optional FIR compiler for direct Dis execution tests")
set(DVM_FIR_SOURCE_COMPILER "" CACHE FILEPATH "Our optional Coco/R numeric source compiler")
if(DVM_RUNTIME_TESTS AND EXISTS "${DVM_FIR_COMPILER}" AND EXISTS "${DVM_FIR_SOURCE_COMPILER}")
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_tables_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_tables.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_tables_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_big_shifts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/big_shifts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_big_shifts_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_table_cfg_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_cfg.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_cfg_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_big_decisions_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/big_decisions.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_big_decisions_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_big_cfg_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_big_cfg.py" "${DVM_FIR_COMPILER}"
      "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_big_cfg_${mode} PROPERTIES TIMEOUT 120
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
    add_test(NAME disvm_fir_dis_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_dis.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_dis_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_cfg_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_cfg.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_cfg_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_reals_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_fir_reals.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_reals_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_limbo_reals_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_limbo_reals.py" "${DVM_FIR_COMPILER}"
      "${DVM_FIR_SOURCE_COMPILER}" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${mode}")
    set_tests_properties(disvm_limbo_reals_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Real projection owner processes; do not confuse this with a network RPC test.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_projection_inspector_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/projection-check.dis" "${mode}")
    set_tests_properties(disvm_projection_inspector_${mode} PROPERTIES TIMEOUT 30)
  endforeach()
endif()

# Optional explicit development tools check the shipped AS client, not a deployed runtime dependency.
set(DVM_IDE_NODE "" CACHE FILEPATH "Node for optional AS client contract checks")
set(DVM_IDE_AS2 "" CACHE FILEPATH "Our Coco/R AS2 syntax frontend for optional client checks")
if(DVM_RUNTIME_TESTS AND DVM_IDE_NODE)
  add_test(NAME disvm_projection_as_client COMMAND "${DVM_IDE_NODE}"
    "${root}/ide/as/test_projection_values.js" "${root}/ide/as/projection-values.as")
endif()
if(DVM_RUNTIME_TESTS AND DVM_IDE_AS2)
  add_test(NAME disvm_projection_as_syntax COMMAND "${DVM_IDE_AS2}" "${root}/ide/as/projection-values.as")
endif()

# The watch panel uses real Dis adapter channels and actual FlashVM raster output, without SDL.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_projection_inspector_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_inspector.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${mode}")
    set_tests_properties(disvm_graphical_projection_inspector_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(feature gutter close)
    foreach(mode jit interpreter)
      add_test(NAME disvm_graphical_${feature}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_${feature}.py" "$<TARGET_FILE:disvm-editor>"
        "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${mode}")
      set_tests_properties(disvm_graphical_${feature}_${mode} PROPERTIES TIMEOUT 120)
    endforeach()
  endforeach()
endif()

# Compile and execute new Limbo table operations on the actual matching DisVM.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_mutation_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_mutation.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_mutation_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Arm-local owners and assignments execute on both actual dispatch paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_arms_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_arms.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_arms_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Run comparison values and eager checked operands on both actual VM dispatch modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_comparisons_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_comparisons.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_comparisons_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Short-circuit joins use actual scalar frame cells on both VM dispatch paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_short_circuit_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/short_circuit.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_short_circuit_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Compound big/table updates execute on both real VM dispatch paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_compound_tables_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/compound_tables.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_compound_tables_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Native Unicode string instructions execute with roots live across one-step GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_unicode_strings_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/unicode_strings.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_unicode_strings_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Native-word execution must retain int32 semantics in both actual VM dispatch modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_int_arithmetic_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/int_arithmetic.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_int_arithmetic_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Rooted concatenations and int bit patterns survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_text_bits_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/text_bits.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_text_bits_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Checked shifts and signed32 results survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_int_shifts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/int_shifts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_int_shifts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Ordered strings and checked int casts survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_text_values_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/text_values.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_text_values_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Complete JIT tails and the direct hot route must share NC64 table/counter policy.
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_native_leaf_test test_native_leaf.c)
  target_include_directories(disvm_native_leaf_test PRIVATE "${u}/include")
  add_test(NAME disvm_complete_tail_nc64_policy COMMAND disvm_native_leaf_test)
endif()

# Ordered constructor fields and key snapshots survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_literals_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_literals.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_literals_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Relative sequence indexes and copies survive one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_relative_sequences_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/relative_sequences.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_relative_sequences_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Reference-compiled native arrays cover every indexed element width and mapped GC owners.
if(DVM_RUNTIME_TESTS)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module" -o "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis"
      "${root}/disVM/examples/relative-sequences/main.b"
    DEPENDS limbo-bootstrap "${root}/disVM/examples/relative-sequences/main.b" VERBATIM)
  add_custom_target(disvm_relative_arrays ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis")
  foreach(mode jit interpreter)
    add_test(NAME disvm_relative_arrays_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/relative-test.dis" "${mode}")
    set_tests_properties(disvm_relative_arrays_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_relative_native_bounds_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_relative_bounds.py" "$<TARGET_FILE:limbo-bootstrap>"
      "$<TARGET_FILE:disvm>" "${u}/module" "${mode}")
    set_tests_properties(disvm_relative_native_bounds_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Real source-to-Dis resized writes, including rune strings and one-instruction GC quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_splice_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_splice.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_splice_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# A handled application error may resume, but policy budgets remain terminal.
if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_hosted_exception_boundary COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_exception_boundary.py" "$<TARGET_FILE:disvm>"
    "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
  set_tests_properties(disvm_hosted_exception_boundary PROPERTIES TIMEOUT 60
    ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
endif()

# Source scopes survive optimization and execute with both scheduler quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_table_leases_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/table_leases.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_table_leases_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()


# FIR ownership dataflow must agree with native recursive alias behavior.
if(DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_lease_flow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/lease_flow.py" "${DVM_FIR_COMPILER}"
      "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_lease_flow_${mode} PROPERTIES TIMEOUT 30)
  endforeach()
endif()

# Source while/for-in must execute cyclic Dis in both VM modes and scheduler quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_source_loops_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/loops.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_source_loops_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_loop_control_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/loop_control.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_loop_control_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_counted_for_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/counted_for.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_counted_for_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

# Preserve source scalar types and captured owners through native cyclic Dis.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_typed_loops_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/typed_loops.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_typed_loops_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# GC roots of private reference cells and captured payloads must survive every quantum.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_retained_loops_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/retained_loops.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_retained_loops_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Execute nonterminal conditions, surviving-arm initialization and scoped transfers.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_function_flow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/function_flow.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_function_flow_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Definite reference stores and canonical nil roots survive GC at both quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_join_initialization_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/join_initialization.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_join_initialization_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Execute original/optimized private scalar cells under both dispatch modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_scalar_forwarding_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/scalar_forwarding.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_scalar_forwarding_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Check original/optimized scalar writes at both dispatch modes and quanta.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_scalar_stores_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/scalar_stores.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_scalar_stores_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Execute Coco/R one-name traversal before/after optimization in both modes.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_value_iteration_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/value_iteration.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" - "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_value_iteration_${mode} PROPERTIES TIMEOUT 90)
  endforeach()
endif()

# Actual selected Regex library covers the full Unicode scalar tag boundary.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_regex_unicode_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/regex-check.dis" "${mode}")
    set_tests_properties(disvm_regex_unicode_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()

# Actual filesystem records and retained Readdir execute through OS-selected ports.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_sys_filesystem_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_filesystem.py" "$<TARGET_FILE:limbo-bootstrap>"
      "$<TARGET_FILE:disvm>" "${u}/module" "${root}/disVM/examples/filesystem/main.b"
      "${CMAKE_CURRENT_BINARY_DIR}/readdir.dis" "${mode}")
    set_tests_properties(disvm_sys_filesystem_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Standalone C89 port contract, independent of either VM heap.
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_filesystem_port_test test_filesystem_port.c)
  target_link_libraries(disvm_filesystem_port_test PRIVATE disvm_shared_ports)
  add_test(NAME disvm_shared_filesystem COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_filesystem_port.py" "$<TARGET_FILE:disvm_filesystem_port_test>")
  set_tests_properties(disvm_shared_filesystem PROPERTIES TIMEOUT 60)
endif()

# Actual typed directory ownership and terminal-browser policy, no mocked Sys I/O.
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_directory_browser_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_directory_browser.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${root}/ide/dis/bridge"
      "${root}/ide/dis/examples/directory-browser/main.b" "${CMAKE_CURRENT_BINARY_DIR}" "${mode}")
    set_tests_properties(disvm_directory_browser_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Graphical browser state lives in Dis; renderer sees only bounded copied labels.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_browser_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_browser.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${mode}")
    set_tests_properties(disvm_graphical_browser_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Optional executable tutorial uses the same generated native ABI and both real dispatch modes.
if(DVM_RUNTIME_NATIVE_DEMO)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${CMAKE_CURRENT_BINARY_DIR}/native-demo.sbl"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module" -I "${native_demo_root}"
      -o "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${native_demo_root}/main.b"
    DEPENDS limbo-bootstrap "${native_demo_root}/nativedemo.m" "${native_demo_root}/main.b" VERBATIM)
  add_custom_target(disvm_native_module_tutorial ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis")
  if(DVM_RUNTIME_TESTS)
    foreach(mode jit interpreter)
      add_test(NAME disvm_native_module_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_native_module.py" "$<TARGET_FILE:disvm>"
        "${CMAKE_CURRENT_BINARY_DIR}/native-demo.dis" "${mode}")
      set_tests_properties(disvm_native_module_${mode} PROPERTIES TIMEOUT 60)
    endforeach()
  endif()
endif()

# Run the new source aliases through our FIR backend and actual native Dis dispatch.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_limbo_aliases_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/limbo_aliases.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_limbo_aliases_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Alias-typed string/key/table lifetimes must survive real native GC backedges.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_alias_owners_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/alias_owners.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_alias_owners_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Actual interpreter/native dispatch must agree on every signed32 bit boundary.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_int_bits_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/int_bits.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_int_bits_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Typed predicates must agree across actual native/interpreter dispatch and hosted tables.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_typed_predicates_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/typed_predicates.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_typed_predicates_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# IEEE ordered predicates must agree in actual native/interpreter and hosted Dis paths.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_real_conditions_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/real_conditions.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_real_conditions_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Exercise lazy numeric conditions in actual interpreter and native dispatch.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_numeric_logic_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/numeric_logic.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_numeric_logic_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Scalar cell updates, joins and numeric loops must execute on the actual VM.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_numeric_flow_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/numeric_flow.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "-" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_numeric_flow_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Service tutorial and independent multi-case acceptance use actual Dis modules.
if(DVM_RUNTIME_COMPILER)
  foreach(example compiler-service compiler-service-check build-panel artifact-export)
    set(source "${root}/ide/dis/examples/compiler-service/main.b")
    if(example STREQUAL "artifact-export")
      set(source "${root}/ide/dis/examples/artifact-export/main.b")
    endif()
    if(example STREQUAL "build-panel")
      set(source "${root}/ide/dis/examples/build-panel/main.b")
    endif()
    if(example STREQUAL "compiler-service-check")
      set(source "${root}/ide/dis/tests/compiler_service_test.b")
    endif()
    add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis" "${CMAKE_CURRENT_BINARY_DIR}/${example}.sbl"
      COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis" "${source}"
      DEPENDS limbo-bootstrap "${source}" ${module_headers} VERBATIM)
    add_custom_target(disvm_${example} ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${example}.dis")
  endforeach()
endif()

if(DVM_RUNTIME_COMPILER AND DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER AND DVM_FIR_SOURCE_COMPILER AND DVM_SWF_LINKER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_compiler_service_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_compiler_service.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/compiler-service-check.dis" ${mode}
      "${DVM_FIR_SOURCE_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_SWF_LINKER}")
    set_tests_properties(disvm_compiler_service_${mode} PROPERTIES TIMEOUT 120)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_compiler_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>"
      ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/compiler-service.dis")
    set_tests_properties(disvm_compiler_tutorial_${mode} PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "PASS compiler tutorial: revision=ok artifact=ok shutdown=ok")
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND NOT DVM_RUNTIME_COMPILER)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis"
      "${root}/ide/dis/tests/compiler_unavailable_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/compiler_unavailable_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_compiler_unavailable ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis")
  foreach(dispatch --jit --no-jit)
    add_test(NAME disvm_compiler_unavailable_${dispatch} COMMAND "$<TARGET_FILE:disvm>"
      ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/compiler-unavailable.dis")
    set_tests_properties(disvm_compiler_unavailable_${dispatch} PROPERTIES TIMEOUT 30
      PASS_REGULAR_EXPRESSION "PASS compiler unavailable: no native fallback, worker=nil, shutdown=ok")
  endforeach()
endif()

# The editor panel executes real native builds and a deterministic concurrent edit.
if(DVM_RUNTIME_COMPILER)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis"
      "${root}/ide/dis/tests/buildpanel_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/buildpanel_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_buildpanel_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis")
  if(DVM_RUNTIME_TESTS)
    foreach(mode jit interpreter)
      set(dispatch --jit)
      if(mode STREQUAL "interpreter")
        set(dispatch --no-jit)
      endif()
      add_test(NAME disvm_buildpanel_${mode} COMMAND "$<TARGET_FILE:disvm>"
        ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/buildpanel-test.dis")
      add_test(NAME disvm_buildpanel_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>"
        ${dispatch} --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/build-panel.dis")
      set_tests_properties(disvm_buildpanel_tutorial_${mode} PROPERTIES TIMEOUT 30
        PASS_REGULAR_EXPRESSION "PASS build panel tutorial: revision=ok binary=ok debug=ok owners=closed")
      set_tests_properties(disvm_buildpanel_${mode} PROPERTIES TIMEOUT 60
        PASS_REGULAR_EXPRESSION "PASS compiler panel: targets=3 copies=ok UTF8=ok stale-native=reject shutdown=ok")
    endforeach()
  endif()
endif()

if(DVM_RUNTIME_COMPILER AND DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_build_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_build.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_graphical_build_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Binary artifact admission uses real channel workers, OS-exclusive files and CLI oracles.
if(DVM_RUNTIME_COMPILER)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis"
      "${root}/ide/dis/tests/artifact_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/artifact_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_artifact_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis")
  if(DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER AND DVM_FIR_SOURCE_COMPILER AND DVM_SWF_LINKER)
    foreach(mode jit interpreter)
      add_test(NAME disvm_artifact_export_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_artifact_export.py" "$<TARGET_FILE:disvm>"
        "${CMAKE_CURRENT_BINARY_DIR}/artifact-test.dis" ${mode}
        "${DVM_FIR_SOURCE_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_SWF_LINKER}")
      set_tests_properties(disvm_artifact_export_${mode} PROPERTIES TIMEOUT 90)
    endforeach()
  endif()
endif()

if(DVM_RUNTIME_TESTS)
  add_test(NAME disvm_exclusive_file_port COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_exclusive_file.py" "${DVM_FLASHVM_SOURCE_ROOT}/ports")
  set_tests_properties(disvm_exclusive_file_port PROPERTIES TIMEOUT 90)
endif()
if(DVM_RUNTIME_COMPILER AND DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_graphical_export_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_graphical_export.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/graphical-editor.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_graphical_export_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis"
      "${root}/ide/dis/app/desktop.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/app/desktop.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_desktop_app ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis")
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -I "${root}/ide/dis/app"
      -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis" "${root}/ide/dis/examples/desktop/main.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop/main.b" "${root}/ide/dis/app/desktop.m" VERBATIM)
  add_custom_target(disvm_desktop_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-example.dis")
endif()

if(DVM_RUNTIME_TESTS)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis"
      "${root}/ide/dis/tests/desktop_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/desktop_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_desktop_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis")
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_owners_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis")
    set_tests_properties(disvm_desktop_owners_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop owners:")
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_movieclip_desktop_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_movieclip_desktop_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()
