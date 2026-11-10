# Experimental native-cell Dis runner; does not reinterpret legacy modules.
cmake_minimum_required(VERSION 3.10)
project(disvm_runtime VERSION 0.121.0 LANGUAGES C)
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RuntimePlatform.cmake")
dvm_runtime_select_backend("${CMAKE_SYSTEM_NAME}" "${CMAKE_SYSTEM_PROCESSOR}"
  "${CMAKE_SIZEOF_VOID_P}" runtime_backend)
include(CheckCSourceCompiles)
if(runtime_backend STREQUAL "arm64")
  check_c_source_compiles("#if !defined(__aarch64__)
#error Selected ARM64 backend requires an AArch64 C compiler
#endif
int main(void) { return 0; }" DVM_RUNTIME_COMPILER_ARM64)
  if(NOT DVM_RUNTIME_COMPILER_ARM64)
    message(FATAL_ERROR "ARM64 runtime selected but the C compiler does not target AArch64")
  endif()
elseif(runtime_backend STREQUAL "riscv64")
  check_c_source_compiles("#if !defined(__riscv) || __riscv_xlen != 64
#error Selected RV64 backend requires an RV64 C compiler
#endif
int main(void) { return 0; }" DVM_RUNTIME_COMPILER_RV64)
  if(NOT DVM_RUNTIME_COMPILER_RV64)
    message(FATAL_ERROR "RV64 runtime selected but the C compiler does not target RV64")
  endif()
else()
  check_c_source_compiles("#if !defined(__x86_64__)
#error Selected AMD64 backend requires an x86-64 C compiler
#endif
int main(void) { return 0; }" DVM_RUNTIME_COMPILER_AMD64)
  if(NOT DVM_RUNTIME_COMPILER_AMD64)
    message(FATAL_ERROR "AMD64 runtime selected but the C compiler does not target x86-64")
  endif()
endif()

message(STATUS "DisVM Linux64 runtime backend: ${runtime_backend}")
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
# Keep setjmp in the original caller while attaching owner-local cleanup marks.
execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${root}/disVM/host/prepare_native_errors.py"
 --source "${u}/emu/port/dat.h" --output "${view}/dat.h" RESULT_VARIABLE error_prepared)
if(NOT error_prepared EQUAL 0)
 message(FATAL_ERROR "Native error checkpoint preparation failed")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
 "${u}/emu/port/dat.h" "${root}/disVM/host/prepare_native_errors.py")

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
# Route hosted C write barriers through the same scalar fields as the collector.
string(APPEND native_interp "\n#ifdef DVM_RUNTIME_GC_FIELDS\n#include \"gc_fields.h\"\n#undef Setmark\n#define Setmark(h) dvm_runtime_gc_mark(&(h)->color, &mutator, &nprop)\n#endif\n")
file(WRITE "${view}/interp.h" "${native_interp}\n#include \"disvm/native_context.h\"\nextern REG *dvm_native_registers(void);\n#define DVM_R (*dvm_native_registers())\nextern int dvm_runtime_leaf(Jit *, int, REG *);\nextern int dvm_runtime_fast(int, int (*)(REG *), REG *);\nextern unsigned long dvm_runtime_instructions;\nextern unsigned long dvm_runtime_instruction_limit;\nextern void dvm_runtime_step(void);\n")
# Source-view regeneration must never silently drop reviewed local adaptations.
file(REMOVE "${view}/prepare.stamp" "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" "${view}/idehostmod.h")

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
file(GLOB runtime_inputs "${u}/libinterp/*.c" "${u}/libinterp/*.h" "${u}/include/isa.h" "${u}/include/interp.h" "${u}/include/vmfetch.h" "${u}/include/vmchannel.h")
# One owning target prepares this mutable source view. Consumers must never
# independently execute the same rewriting command in parallel Make subgraphs.
add_custom_command(OUTPUT "${view}/prepare.stamp"
 BYPRODUCTS "${view}/exception.c" "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" "${view}/bigtablemod.h" "${view}/processctlmod.h" ${host_prepare_products}
 COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare.py"
   --compiler "$<TARGET_FILE:limbo-bootstrap>" --sys "${u}/module/sys.m" ${host_prepare_args} --source "${u}/libinterp" --output "${view}"
 COMMAND "${CMAKE_COMMAND}" -E touch "${view}/prepare.stamp"
 DEPENDS limbo-bootstrap prepare.py prepare_channel_new.py prepare_record_new.py record_new.h prepare_mapped_cons.py mapped_cons.h prepare_raw_cons.py raw_cons.h prepare_scalar_cons.py scalar_cons.h "${root}/disVM/examples/native-module/nativedemo.m" ${DVM_LIMBO_BINDINGS} "${root}/disVM/module/bigtable.m" "${root}/disVM/module/processctl.m" ${runtime_inputs} "${root}/ide/dis/bridge/idehost.m" "${root}/disVM/module/fircompiler.m" native_tables.c relative_sequence.c prepare_exception.py prepare_type_counts.py prepare_heap_counts.py prepare_opcode_counts.py prepare_module_counts.py prepare_snapshots.py prepare_observations.py prepare_jit_counts.py prepare_pointer_builder.py prepare_pointer_tail.py prepare_owner_tail.py prepare_aggregate_tail.py prepare_shared_pointer.py prepare_shared_bytes.py prepare_shared_mapped.py shared_mapped.h prepare_shared_head.py shared_head.h prepare_shared_list_pointer.py shared_list_pointer.h prepare_shared_list_bytes.py shared_list_bytes.h prepare_shared_integer.py shared_integer.h prepare_shared_arithmetic.py shared_arithmetic.h prepare_shared_shift.py shared_shift.h prepare_shared_branch.py shared_branch.h prepare_shared_array_length.py shared_array_length.h prepare_shared_real_conversion.py shared_real_conversion.h prepare_shared_real_arithmetic.py shared_real_arithmetic.h prepare_shared_real_branch.py shared_real_branch.h prepare_shared_string_branch.py shared_string_branch.h prepare_shared_string_length.py shared_string_length.h prepare_shared_list_length.py shared_list_length.h prepare_bytes_tail.py shared_bytes.h bytes_fast.h shared_pointer.h aggregate_fast.h owner_fast.h prepare_roots.py prepare_boundary_counts.py prepare_stack_check.py prepare_gc_fields.py "${u}/include/interp.h" root_registry.h pointer_builder.h heap_snapshot.h module_counts.h heap_counts.h type_counts.h exception_host.h table_leases.h "${u}/emu/port/exception.c" "${root}/common/table/relative.h" "${u}/module/sys.m" "${u}/module/srv.m" "${u}/module/loader.m" "${u}/module/math.m" "${u}/emu/port/srv.c" VERBATIM)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runtime_inputs}
  "${root}/disVM/host/prepare_native_registers.py")
set(core alt conv dec disc exception gc heap link load loader math raise readmod runt stack string validstk xec comp-${runtime_backend})
set(sources)
foreach(unit IN LISTS core)
 list(APPEND sources "${view}/${unit}.c")
endforeach()
add_custom_target(disvm_runtime_sources DEPENDS "${view}/prepare.stamp")
add_executable(disvm
    "${root}/disVM/host/refcount.c"
    "${root}/disVM/host/object_guard.c"
    "${root}/disVM/host/guard_registry.c"
    "${root}/disVM/host/cell_access.c"
    "${root}/disVM/host/range_access.c"
    "${root}/disVM/host/small_range.c"
    "${root}/disVM/host/owner_scope.c"
    "${root}/disVM/host/owner_slot.c"
    main.c
    native_jump.c
    runqueue.c
    owner_messages.c
    thread_registrar.c
    queue_probe.c
    managed_workers.c
    process_environment.c
    file_gate.c
    seek_sys.c
    metadata_wait.c
    open_wait.c
    close_worker.c
    runtime_gc.c
    runtime_rc.c
    gc/common/graph_storage.c
    gc/common/collector_environment.c
    gc/common/native_admission.c
    gc/fast/retirement_pass.c
    gc/common/process_roots.c
    gc/fast/graph_confirmation.c
    gc/fast/packet_budget.c
    gc/common/graph_capture.c
    gc/cycles/cycle_pass.c
    gc/duke/snapshot_growth.c
    gc/duke/large_graph_pass.c
    retirement_queue.c
    gc_fields.c
    type_counts.c
    type_counts_probe.c
    heap_counts.c
    heap_counts_probe.c
    opcode_counts_probe.c
    module_counts.c
    module_counts_probe.c
    heap_snapshot.c
    heap_pool.c
    root_registry.c
    root_batch.c
    root_registry_probe.c
    pointer_stage.c
    record_new.c
    record_storage.c
    mapped_cons.c
    mapped_prepare.c
    raw_cons.c
    scalar_cons.c
    scalar_tail.c
    value_prepare.c
    pointer_transaction.c
    pointer_prepare.c
    pointer_builder.c
    owner_fast.c
    shared_pointer.c
    shared_access.c
    shared_bytes.c
    shared_mapped.c
    heap_commit.c
    heap_hold.c
    list_hold.c
    shared_head.c
    shared_list_pointer.c
    shared_list_bytes.c
    shared_integer.c
    integer_value.c
    arithmetic_value.c
    shared_arithmetic.c
    exception_reserve.c
    shared_shift.c
    shift_value.c
    shared_branch.c
    branch_value.c
    shared_array_length.c
    array_length_value.c
    shared_real_conversion.c
    real_conversion_value.c
    shared_real_arithmetic.c
    real_arithmetic_value.c
    shared_real_branch.c
    real_branch_value.c
    shared_string_branch.c
    string_branch_value.c
    shared_string_length.c
    string_length_value.c
    shared_list_length.c
    list_length_value.c
    byte_transfer.c
    bytes_fast.c
    aggregate_fast.c
    pointer_builder_probe.c
    heap_snapshot_probe.c
    service_counts_probe.c
    managed_owner.c
    managed_owner_probe.c
    guard_registry_probe.c
    sleep_sys.c
    read_sys.c
    write_sys.c
    formatted_sys.c
    io_buffer.c
    clock_sys.c
    sys_capture.c
    filesystem_sys.c
    srv_adapter.c
    bigtable.c
    processctl.c
    "${u}/libmath/blas.c"
    "${u}/libmath/gemm.c"
    "${u}/libmath/pow10.c"
    "${view}/srv.c"
    "${view}/unsupported.c"
    ${sources}
    "${root}/disVM/host/native_registers.c"
)
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
target_compile_definitions(disvm PRIVATE DVM_NATIVE_REGISTERS=1 DVM_RUNTIME_GC_FIELDS=1 DVM_RUNTIME_VERSION="${PROJECT_VERSION}" DVM_RUNTIME_BACKEND="${runtime_backend}")
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
set(DVM_COCOR_LIMBO_ROOT "${root}/tools/coco-r-limbo" CACHE PATH "Explicit Coco/R Limbo generator source")
if(NOT EXISTS "${DVM_COCOR_LIMBO_ROOT}/CMakeLists.txt")
  message(FATAL_ERROR "Provide DVM_COCOR_LIMBO_ROOT containing the source-built generator")
endif()
add_subdirectory("${DVM_COCOR_LIMBO_ROOT}" ide-cocor)
file(GLOB syntax_frames "${DVM_COCOR_LIMBO_ROOT}/upstream/frames/*.frm")
set(syntax_products)
foreach(profile limbo as)
  set(grammar "${root}/ide/syntax/${profile}lex.atg")
  set(generated "${CMAKE_CURRENT_BINARY_DIR}/syntax/${profile}lex.b")
  set(binary "${CMAKE_CURRENT_BINARY_DIR}/${profile}lex.dis")
  add_custom_command(OUTPUT "${generated}"
    COMMAND "${PYTHON_EXECUTABLE}" "${root}/ide/syntax/generate.py"
      --generator "$<TARGET_FILE:cocor>" --frames "${DVM_COCOR_LIMBO_ROOT}/upstream/frames"
      --grammar "${grammar}" --output "${generated}"
    DEPENDS cocor "${grammar}" "${root}/ide/syntax/generate.py" ${syntax_frames} VERBATIM)
  add_custom_command(OUTPUT "${binary}"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module" -o "${binary}" "${generated}"
    DEPENDS limbo-bootstrap "${generated}" "${u}/module/sys.m" "${u}/module/draw.m" VERBATIM)
  list(APPEND syntax_products "${binary}")
endforeach()
set(includes -I "${u}/module" -I "${root}/ide/dis" -I "${root}/ide/dis/bridge"
  -I "${root}/ide/syntax"
  -I "${root}/ide/dis/examples/workspace" -I "${root}/disVM/module" -I "${root}/disVM/broker")
set(module_names ideatlas idedeskclip idecore ideworkspace idecompiler idebuildpanel ideartifact idedeskpane idedesktop workspace-example ide-console console-check uibridge ideinput idebridge idecomplete idebuiltins editor-console idefiles bufio string arg readdir debug dis styx styxservers libraries-check ideviewport idefileops idesearch decimal-append ideclipboard idefield field-demo brokerdiag ideprojection projection-check ideinspect idevalue idedocprojection regex regex-check idebrowser)
set(module_sources "${root}/ide/dis/bridge/ideatlas.b" "${root}/ide/dis/bridge/idedeskclip.b" "${root}/ide/dis/idecore.b" "${root}/ide/dis/bridge/ideworkspace.b"
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
list(APPEND module_headers "${root}/ide/dis/tests/idevectoredit_checks.b" "${root}/ide/dis/tests/idevectornodes_checks.b" "${root}/ide/dis/tests/idevectorclip_checks.b" "${root}/ide/dis/tests/idevectorfit_checks.b" "${root}/ide/dis/bridge/idevectorwire.b" "${root}/ide/dis/bridge/idevectorpresentation.b" "${root}/ide/dis/bridge/idevectorchain.b" "${root}/ide/dis/bridge/idevectoredit.b" "${root}/ide/dis/bridge/idevectorbrushfit.b" "${root}/ide/dis/bridge/idevectorpick.b" "${root}/ide/dis/bridge/idevectorcancel.b" "${root}/ide/dis/bridge/idevectorlocal.b" "${root}/ide/dis/bridge/idevectornodes.b" "${root}/ide/dis/bridge/idevectornodecurve.b" "${root}/ide/dis/bridge/idevectornodebreak.b" "${root}/ide/dis/bridge/idevectoroverlaycache.b" "${root}/ide/dis/bridge/idevectorclipboard.b" "${root}/ide/dis/bridge/ideinkacquire.b" "${root}/ide/dis/bridge/ideinkfit.b" "${root}/ide/dis/bridge/idevectorviewport.b" "${root}/ide/dis/bridge/idedesktopatlas.b" "${root}/ide/dis/bridge/idedesktoptools.b")
list(APPEND module_names capture-demo asincviews ideinkworker idemdi iderails idetools idetoolpalette iderecent idepick idethumbnails idetabview idedocking idesourcedomain idedebug vector-demo idevector idesyntax idetokenquery syntax-check tokenquery-demo)
list(APPEND module_sources "${root}/disVM/examples/asincviews/main.b" "${root}/disVM/limbo/asincviews.b" "${root}/ide/dis/bridge/ideinkworker.b" "${root}/ide/dis/bridge/idemdi.b" "${root}/ide/dis/bridge/iderails.b" "${root}/ide/dis/bridge/idetools.b" "${root}/ide/dis/bridge/idetoolpalette.b" "${root}/ide/dis/bridge/iderecent.b" "${root}/ide/dis/bridge/idepick.b" "${root}/ide/dis/bridge/idethumbnails.b" "${root}/ide/dis/bridge/idetabview.b" "${root}/ide/dis/bridge/idedocking.b" "${root}/ide/dis/bridge/idesourcedomain.b" "${root}/ide/dis/bridge/idedebug.b" "${root}/ide/dis/examples/vector/main.b" "${root}/ide/dis/bridge/idevector.b" "${root}/ide/dis/bridge/idesyntax.b" "${root}/ide/dis/bridge/idetokenquery.b" "${root}/ide/dis/tests/idesyntax_test.b" "${root}/ide/dis/examples/token-query/main.b")
# Portable cooperative task ownership is a library and separately runnable example.
list(APPEND module_names idevectorlabels idebulk bulkports-test asinctasks tasks-demo)
list(APPEND module_sources
  "${root}/ide/dis/bridge/idevectorlabels.b"
  "${root}/ide/dis/bridge/idebulk.b"
  "${root}/ide/dis/tests/idebulk_test.b"
  "${root}/disVM/limbo/asinctasks.b"
  "${root}/disVM/examples/asinctasks/main.b")
file(GLOB syntax_headers "${root}/ide/syntax/*.m")
list(APPEND module_headers ${syntax_headers})
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
add_custom_target(disvm_native_modules ALL DEPENDS ${products} ${syntax_products})
option(DVM_RUNTIME_TESTS "Run real native Dis IDE service regressions" ON)
if(DVM_RUNTIME_TESTS)
  enable_testing()
  foreach(mode jit interpreter)
    add_test(NAME disvm_tokenquery_example_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/tokenquery-demo.dis" ${mode})
    set_tests_properties(disvm_tokenquery_example_${mode} PROPERTIES TIMEOUT 60)
  endforeach()
endif()
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
 include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Native64Review.cmake")
 add_test(NAME disvm_runtime_platform_selection COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_platform.py" "${CMAKE_COMMAND}"
   "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RuntimePlatform.cmake")
 foreach(test asinctasks asincviews idemdi idedebug idevectorpane idevector idepaint idetokenquery idesyntaxincremental idesyntax idecore limits coordinates find history replaceall utf8span ideworkspace idebridge idecomplete idebuiltins navigation viewport uicodec words indent decimalappend clipboard field stringslice values docprojection hybridtable tableguards tableleases)
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
  # Geometry regression includes multiple whole-page union/contour passes.
  if(test STREQUAL "idepaint")
    set(application_budget 120)
  endif()
  if(test STREQUAL "idesyntaxincremental")
    set(application_budget 120)
  endif()
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
  foreach(mode jit interpreter)
    add_test(NAME disvm_asinctasks_quantum_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/asinctasks-test.dis" "${mode}" 50 1)
    set_tests_properties(disvm_asinctasks_quantum_${mode} PROPERTIES TIMEOUT 60)
    add_test(NAME disvm_asinctasks_limit_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_asinctasks_limit.py" "$<TARGET_FILE:disvm>"
      "$<TARGET_FILE:limbo-bootstrap>" "${root}/disVM/module" "${u}/module"
      "${root}/disVM/limbo/asinctasks.b" "${root}/ide/dis/tests/asinctaskslimit_test.b" "${mode}")
    set_tests_properties(disvm_asinctasks_limit_${mode} PROPERTIES TIMEOUT 130
      ENVIRONMENT "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/test-tmp")
  endforeach()
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
  add_executable(disvm-editor ${runtime_sources} editor_host.c editor_thread.c editor_bulk.c editor_view.c editor_raster.c editor_stamp.c editor_ink.c
    "${root}/disVM/host/dis_ui_channel.c")
  if(DVM_RUNTIME_TESTS)
    add_executable(disvm_editor_raster_test test_editor_raster.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_editor_raster_test PRIVATE flashvm)
    add_executable(disvm_frame_free_drawing_test test_frame_free_drawing.c editor_raster.c editor_stamp.c editor_ink.c)
    target_include_directories(disvm_frame_free_drawing_test PRIVATE "${DVM_FLASHVM_SOURCE_ROOT}/ports")
    target_link_libraries(disvm_frame_free_drawing_test PRIVATE flashvm m)
    add_test(NAME disvm_frame_free_drawing COMMAND disvm_frame_free_drawing_test)
    add_executable(disvm_movieclip_capture_test test_movieclip_capture.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_movieclip_capture_test PRIVATE flashvm m)
    add_test(NAME disvm_movieclip_native_capture COMMAND disvm_movieclip_capture_test)
    set_tests_properties(disvm_movieclip_native_capture PROPERTIES TIMEOUT 30)
    add_executable(disvm_retained_scene_test test_retained_scene.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_retained_scene_test PRIVATE flashvm m)
    add_test(NAME disvm_editor_retained_scene COMMAND disvm_retained_scene_test)
    add_executable(disvm_editor_incremental_test test_editor_incremental.c editor_raster.c editor_stamp.c editor_ink.c)
    target_link_libraries(disvm_editor_incremental_test PRIVATE flashvm m)
    add_test(NAME disvm_editor_dirty_parallel_raster COMMAND disvm_editor_incremental_test)
    set_tests_properties(disvm_editor_dirty_parallel_raster PROPERTIES TIMEOUT 30)
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
  # Each chrome view is a separate Coco/R AS -> FIR -> optimizer -> SWL product.
  set(chrome_headers)
  foreach(control desktop-background rail-left rail-right window-card miniature-card window-header tab-button close-button tool-scroll tool-palette tool-pointer input-field top-bar vector-control atlas-card atlas-frame atlas-miniature)
    string(REPLACE "-" "_" symbol "chrome_${control}")
    set(header "${view}/${symbol}.h")
    add_custom_command(OUTPUT "${header}"
      COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/ide/as/chrome/${control}.as" "${view}/${control}.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_COMPILER}" check "${view}/${control}.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_OPTIMIZER}" "${view}/${control}.fir" "${view}/${control}.opt.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_COMPILER}" object "${view}/${control}.opt.fir" "${view}/${control}.swo"
      COMMAND "${DVM_SWF_LINKER}" link "${view}/${control}.swf" "${view}/${control}.swo"
      COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/embed_chrome.py" "${view}/${control}.swf" "${header}" "${symbol}"
      DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_FIR_OPTIMIZER}" "${DVM_SWF_LINKER}"
        "${root}/ide/as/chrome/${control}.as" embed_chrome.py VERBATIM)
    list(APPEND chrome_headers "${header}")
  endforeach()
  add_custom_target(disvm_chrome_headers DEPENDS ${chrome_headers})
  if(DVM_RUNTIME_TESTS)
    add_custom_command(OUTPUT "${view}/native-module.swf"
      COMMAND "${DVM_AS_VIEW_COMPILER}" "${root}/flashVM/examples/embedding/native_module/main.as" "${view}/native-module.fir"
      COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${CMAKE_CURRENT_BINARY_DIR}/tmp" "${DVM_FIR_COMPILER}" object "${view}/native-module.fir" "${view}/native-module.swo"
      COMMAND "${DVM_SWF_LINKER}" link "${view}/native-module.swf" "${view}/native-module.swo"
      DEPENDS "${DVM_AS_VIEW_COMPILER}" "${DVM_FIR_COMPILER}" "${DVM_SWF_LINKER}" "${root}/flashVM/examples/embedding/native_module/main.as" VERBATIM)
    add_custom_target(disvm_flashvm_native_module_swf DEPENDS "${view}/native-module.swf")
    add_executable(disvm_flashvm_native_module_example
      "${root}/flashVM/examples/embedding/native_module/main.c" "${root}/flashVM/examples/embedding/native_module/module.c")
    set_property(TARGET disvm_flashvm_native_module_example PROPERTY C_STANDARD 90)
    target_link_libraries(disvm_flashvm_native_module_example PRIVATE flashvm m)
    add_dependencies(disvm_flashvm_native_module_example disvm_flashvm_native_module_swf)
    add_test(NAME disvm_flashvm_native_module_example COMMAND disvm_flashvm_native_module_example "${view}/native-module.swf")
  endif()
  # One producer owns both generated headers across parallel Make target graphs.
  add_custom_target(disvm_view_headers DEPENDS
    "${view}/ide_view_swf.h" "${view}/desktop_view_swf.h")
  add_dependencies(disvm-editor disvm_view_headers)
  target_sources(disvm-editor PRIVATE desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c)
  if(DVM_RUNTIME_TESTS)
    add_executable(disvm_desktop_colors_test test_desktop_colors.c desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c editor_raster.c editor_stamp.c editor_ink.c)
    add_dependencies(disvm_desktop_colors_test disvm_view_headers)
    target_include_directories(disvm_desktop_colors_test PRIVATE "${view}" "${root}/disVM/include")
    target_link_libraries(disvm_desktop_colors_test PRIVATE flashvm m)
    add_test(NAME disvm_desktop_color_protocol COMMAND disvm_desktop_colors_test)
    add_executable(disvm_mdi_capture_test test_mdi_capture.c desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c editor_raster.c editor_stamp.c editor_ink.c)
    add_dependencies(disvm_mdi_capture_test disvm_view_headers)
    target_include_directories(disvm_mdi_capture_test PRIVATE "${view}" "${root}/disVM/include")
    target_link_libraries(disvm_mdi_capture_test PRIVATE flashvm m)
    if(DVM_EDITOR_TEST_FONT)
      add_test(NAME disvm_mdi_reflow_capture COMMAND disvm_mdi_capture_test "${DVM_EDITOR_TEST_FONT}")
    endif()
    add_executable(disvm_desktop_vector_test test_desktop_vector.c desktop_view.c vector_local.c vector_transform.c desktop_paint_primitives.c desktop_drawing_capability.c desktop_window_chrome.c editor_raster.c editor_stamp.c editor_ink.c)
    add_dependencies(disvm_desktop_vector_test disvm_view_headers)
    target_include_directories(disvm_desktop_vector_test PRIVATE "${view}" "${root}/disVM/include")
    target_link_libraries(disvm_desktop_vector_test PRIVATE flashvm m)
    add_test(NAME disvm_desktop_vector_protocol COMMAND disvm_desktop_vector_test)
    add_executable(disvm_as_view_test test_as_view.c)
    add_dependencies(disvm_as_view_test disvm_view_headers)
    target_include_directories(disvm_as_view_test PRIVATE "${view}")
    target_link_libraries(disvm_as_view_test PRIVATE flashvm)
    add_test(NAME disvm_as_view_execution COMMAND disvm_as_view_test)
  endif()
  target_include_directories(disvm-editor PRIVATE ${runtime_includes} "${DVM_FLASHVM_SOURCE_ROOT}/ports")
  target_compile_definitions(disvm-editor PRIVATE DVM_NATIVE_REGISTERS=1 DVM_RUNTIME_GC_FIELDS=1 DVM_RUNTIME_EDITOR=1 DVM_RUNTIME_VERSION="${PROJECT_VERSION}" DVM_RUNTIME_BACKEND="${runtime_backend}")
  target_compile_options(disvm-editor PRIVATE -fno-strict-aliasing)
  target_link_libraries(disvm-editor PRIVATE fvm_owned_table disvm_flashvm_host limbo_bootstrap_lib9
    limbo_bootstrap_support limbo_bootstrap_lib9 flashvm m)
  if(DVM_EDITOR_SDL)
    find_package(SDL2 2.0.9 CONFIG REQUIRED)
    target_sources(disvm-editor PRIVATE editor_desktop_input.c editor_motion_batch.c)
    if(DVM_RUNTIME_TESTS)
      add_executable(disvm_paint_input_test test_paint_input.c editor_desktop_input.c)
      target_link_libraries(disvm_paint_input_test PRIVATE SDL2::SDL2)
      add_test(NAME disvm_paint_input_queue COMMAND disvm_paint_input_test)
    endif()
    target_compile_definitions(disvm-editor PRIVATE DVM_EDITOR_SDL=1)
    target_sources(disvm-editor PRIVATE editor_window_events.c editor_text_events.c)
    if(DVM_RUNTIME_TESTS)
      add_executable(disvm_text_events_test test_text_events.c editor_text_events.c)
      target_include_directories(disvm_text_events_test PRIVATE "${root}/disVM/include" "${DVM_FLASHVM_SOURCE_ROOT}/include")
      target_link_libraries(disvm_text_events_test PRIVATE SDL2::SDL2 flashvm)
      add_test(NAME disvm_sdl_text_bursts COMMAND disvm_text_events_test)
      add_executable(disvm_window_events_test test_window_events.c editor_window_events.c)
      target_include_directories(disvm_window_events_test PRIVATE "${root}/disVM/include" "${DVM_FLASHVM_SOURCE_ROOT}/include")
      target_link_libraries(disvm_window_events_test PRIVATE SDL2::SDL2)
      if(UNIX AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        add_executable(disvm_desktop_touch_test test_desktop_touch.c editor_desktop_input.c)
        add_dependencies(disvm_desktop_touch_test disvm_view_headers)
        get_target_property(editor_includes disvm-editor INCLUDE_DIRECTORIES)
        target_include_directories(disvm_desktop_touch_test PRIVATE ${editor_includes} "${DVM_FLASHVM_SOURCE_ROOT}/include")
        target_compile_definitions(disvm_desktop_touch_test PRIVATE DVM_EDITOR_SDL=1)
        target_compile_options(disvm_desktop_touch_test PRIVATE -ffunction-sections -fdata-sections)
        target_link_options(disvm_desktop_touch_test PRIVATE -Wl,--gc-sections)
        target_link_libraries(disvm_desktop_touch_test PRIVATE SDL2::SDL2 m)
        add_test(NAME disvm_desktop_native_contacts COMMAND disvm_desktop_touch_test)
      endif()
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

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
    -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis" "${root}/ide/dis/examples/desktop-lifecycle/main.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-lifecycle/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_lifecycle ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_lifecycle_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-lifecycle.dis")
    set_tests_properties(disvm_desktop_lifecycle_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop lifecycle tutorial:")
  endforeach()
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
      --quantum 1 --max-instructions 2000000 "${CMAKE_CURRENT_BINARY_DIR}/desktop-test.dis")
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

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_pointer_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_pointer.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_pointer_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
    -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis" "${root}/ide/dis/examples/desktop-pointer/main.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-pointer/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_pointer_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_pointer_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-pointer.dis")
    set_tests_properties(disvm_desktop_pointer_tutorial_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop pointer tutorial:")
  endforeach()
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
    -o "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis" "${root}/ide/dis/examples/desktop-clipboard/main.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/examples/desktop-clipboard/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_desktop_clipboard_example ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_clipboard_tutorial_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 "${CMAKE_CURRENT_BINARY_DIR}/desktop-clipboard.dis")
    set_tests_properties(disvm_desktop_clipboard_tutorial_${mode} PROPERTIES TIMEOUT 60
      PASS_REGULAR_EXPRESSION "PASS desktop clipboard tutorial:")
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_clipboard_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_clipboard.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_clipboard_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis"
      "${root}/ide/dis/tests/deskclip_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/deskclip_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_deskclip_check ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis")
  foreach(mode jit interpreter)
    set(dispatch --jit)
    if(mode STREQUAL "interpreter")
      set(dispatch --no-jit)
    endif()
    add_test(NAME disvm_desktop_clipboard_guards_${mode} COMMAND "$<TARGET_FILE:disvm>" ${dispatch}
      --quantum 1 --max-instructions 50000000 "${CMAKE_CURRENT_BINARY_DIR}/deskclip-test.dis")
    set_tests_properties(disvm_desktop_clipboard_guards_${mode} PROPERTIES TIMEOUT 90
      PASS_REGULAR_EXPRESSION "PASS desktop clipboard guards:")
  endforeach()
endif()

# Exercise normalized gestures against actual Dis policy and MovieClip pixels.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS AND DVM_EDITOR_TEST_FONT)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_retained_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_retained.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_retained_${mode} PROPERTIES TIMEOUT 90)
    add_test(NAME disvm_desktop_pool_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_pool.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_pool_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_rails_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_rails.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_rails_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_mdi_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_mdi.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_mdi_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_debugger_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_debugger.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_debugger_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_vector_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_vector.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_vector_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_vector_mouse_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_vector.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode} mouse)
    set_tests_properties(disvm_desktop_vector_mouse_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_desktop_syntax_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_syntax.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" ${mode})
    set_tests_properties(disvm_desktop_syntax_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_focus_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_focus.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_desktop_focus_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_workspace_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_workspace.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_desktop_workspace_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_desktop_glass_motion_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_glass_motion.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_desktop_glass_motion_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Run raw/optimized integer identities in the actual matched Dis engines.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_integer_identities_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_identities.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_identities_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Cross-target checked shift admission shares the production FIR libraries.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_swf_int_shifts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/swf_int_shifts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_swf_int_shifts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# The same signed int32 division/remainder results must reach both actual engines.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_swf_int_divmod_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/swf_int_divmod.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_swf_int_divmod_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_total_arithmetic_dce_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/total_arithmetic_dce.py" "${DVM_FIR_COMPILER}"
      "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_total_arithmetic_dce_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Build the same structural codec tools beside the separate NC64 runtime.
add_subdirectory("${root}/disVM/envelope" envelope)

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_integer_pairs_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_pairs.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_pairs_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interp)
    add_test(NAME disvm_fir_integer_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_facts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interp)
    add_test(NAME disvm_fir_integer_shift_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_shift_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_shift_facts_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Frame-free engine scheduling and SDL text-burst boundaries use real owner APIs.
if(DVM_RUNTIME_TESTS AND TARGET flashvm)
  foreach(name async timers timers_budget timers_next_delay timers_versions_conflicts stage_update)
    add_executable(disvm_flashvm_${name}_test "${DVM_FLASHVM_SOURCE_ROOT}/tests/${name}.cpp")
    target_link_libraries(disvm_flashvm_${name}_test PRIVATE flashvm)
    add_test(NAME disvm_flashvm_${name} COMMAND disvm_flashvm_${name}_test)
  endforeach()
endif()

if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interp)
    add_test(NAME disvm_fir_integer_cast_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_cast_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_cast_facts_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_integer_power_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_power_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_power_facts_${mode} PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_fir_integer_known_results_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_known_results.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_integer_known_results_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Real owning-compiler regression: preserve old array values through initializer
# evaluation and publish the completed temporary only afterwards. Both optimizer
# selections execute on the matching native-cell VM in interpreter/JIT modes.
if(DVM_RUNTIME_TESTS)
  foreach(selection plain optimized)
    set(alias_flags)
    if(selection STREQUAL "optimized")
      set(alias_flags -O)
    endif()
    set(alias_binary "${CMAKE_CURRENT_BINARY_DIR}/array-alias-${selection}.dis")
    add_custom_command(OUTPUT "${alias_binary}"
      COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${alias_flags} ${includes} -o "${alias_binary}"
        "${u}/utils/tests/array-initializer-alias.b"
      DEPENDS limbo-bootstrap "${u}/utils/tests/array-initializer-alias.b" VERBATIM)
    add_custom_target(disvm_array_alias_${selection} ALL DEPENDS "${alias_binary}")
    foreach(mode jit interpreter)
      add_test(NAME disvm_array_initializer_${selection}_${mode} COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>" "${alias_binary}" ${mode})
      set_tests_properties(disvm_array_initializer_${selection}_${mode} PROPERTIES TIMEOUT 60)
    endforeach()
  endforeach()
endif()

# Every consumer of desktop_view links the same generic primitives and AS resources.
if(TARGET disvm_chrome_headers)
  foreach(consumer disvm-editor disvm_desktop_colors_test disvm_mdi_capture_test disvm_desktop_vector_test)
    if(TARGET ${consumer})
      add_dependencies(${consumer} disvm_chrome_headers)
    endif()
  endforeach()
endif()

# Run the new proof through ordinary interpreter/native JIT execution, not a model.
if(DVM_RUNTIME_TESTS AND DVM_FIR_SOURCE_COMPILER AND DVM_FIR_COMPILER)
  foreach(mode jit interpreter)
    add_test(NAME disvm_fir_sparse_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_sparse_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_sparse_facts_${mode} PROPERTIES TIMEOUT 180)
    add_test(NAME disvm_fir_division_facts_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${root}/fir/v1/tests/integer_division_facts.py" "${DVM_FIR_SOURCE_COMPILER}"
      "${DVM_FIR_COMPILER}" "$<TARGET_FILE:disvm>" "${mode}")
    set_tests_properties(disvm_fir_division_facts_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# Reuse the real editor configuration with a fixture entry replacing only main.
# The copied native buffers are consumed by the production Limbo geometry worker.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  get_target_property(ink_worker_sources disvm-editor SOURCES)
  list(REMOVE_ITEM ink_worker_sources main.c)
  add_executable(disvm_native_ink_runner test_ink_runner.c ${ink_worker_sources})
  if(DVM_EDITOR_SDL)
    target_sources(disvm_native_ink_runner PRIVATE test_live_ink.c)
  endif()
  foreach(property INCLUDE_DIRECTORIES COMPILE_DEFINITIONS COMPILE_OPTIONS LINK_OPTIONS)
    get_target_property(ink_worker_property disvm-editor ${property})
    if(ink_worker_property)
      set_property(TARGET disvm_native_ink_runner PROPERTY ${property} "${ink_worker_property}")
    endif()
  endforeach()
  get_target_property(ink_worker_libraries disvm-editor LINK_LIBRARIES)
  target_link_libraries(disvm_native_ink_runner PRIVATE ${ink_worker_libraries})
  add_dependencies(disvm_native_ink_runner disvm_runtime_sources disvm_view_headers disvm_chrome_headers)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis"
    COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g ${includes}
      -o "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis" "${root}/ide/dis/tests/native_ink_test.b"
    DEPENDS limbo-bootstrap "${root}/ide/dis/tests/native_ink_test.b" ${module_headers} VERBATIM)
  add_custom_target(disvm_native_ink_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis")
  foreach(mode jit interpreter)
    add_test(NAME disvm_native_ink_worker_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_native_ink.py" "$<TARGET_FILE:disvm_native_ink_runner>"
      "${CMAKE_CURRENT_BINARY_DIR}/native-ink-test.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_native_ink_worker_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(TARGET disvm_native_ink_runner AND DVM_EDITOR_SDL)
  add_test(NAME disvm_native_ink_sdl COMMAND disvm_native_ink_runner --sdl-fixture "${DVM_EDITOR_TEST_FONT}")
  set_tests_properties(disvm_native_ink_sdl PROPERTIES TIMEOUT 60
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_maximize_native_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_maximize_native.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_maximize_native_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  add_executable(disvm_ink_primitives_test test_ink_primitives.c editor_ink.c editor_stamp.c)
  target_link_libraries(disvm_ink_primitives_test PRIVATE flashvm m)
  add_test(NAME disvm_native_ink_primitives COMMAND disvm_ink_primitives_test)
endif()

# Scroll batching preserves exact legacy distance and returned viewport pixels.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_scroll_batch_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_scroll_batch.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_scroll_batch_${mode} PROPERTIES TIMEOUT 240)
  endforeach()
endif()

# Tool-state transitions must not reserialize/rasterize already committed vectors.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_control_cache_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_controls_cache.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_vector_control_cache_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

# Two real document owners must make progress through separate geometry queues.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_bulk_ports_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/bulkports-test.dis" "${mode}")
    set_tests_properties(disvm_bulk_ports_${mode} PROPERTIES TIMEOUT 60
      ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_test(NAME disvm_bulk_geometry_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_bulk_transport.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_bulk_geometry_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# Ordinary full-frame commands must preserve open strokes and compound fill rules.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  add_executable(disvm_full_strokes_test test_full_strokes.c editor_raster.c editor_stamp.c)
  target_link_libraries(disvm_full_strokes_test PRIVATE flashvm m)
  add_test(NAME disvm_full_strokes COMMAND disvm_full_strokes_test)
endif()

# Independent committed Line and Pen colors must survive real authoring and SVG save.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_tool_modes_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_tool_modes.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_vector_tool_modes_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# The finite overview is a real fixed MovieClip layer with live source mirrors.
if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_desktop_atlas_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_desktop_atlas.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${DVM_EDITOR_TEST_FONT}" "${mode}")
    set_tests_properties(disvm_desktop_atlas_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_editing_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_editing.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_vector_editing_${mode} PROPERTIES TIMEOUT 120)
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_viewport_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_viewport.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_vector_viewport_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis"
    "${root}/ide/dis/tests/millisec_test.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/tests/millisec_test.b" VERBATIM)
add_custom_target(disvm_millisec_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis")
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_clock_port_test test_clock_port.c)
  target_link_libraries(disvm_clock_port_test PRIVATE disvm_flashvm_host)
  add_test(NAME disvm_monotonic_clock_port COMMAND disvm_clock_port_test)
  foreach(mode jit interpreter)
    add_test(NAME disvm_millisec_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/millisec-test.dis" ${mode})
  endforeach()
endif()

if(DVM_RUNTIME_EDITOR AND DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_vector_dense_selection_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_vector_dense_selection.py" "$<TARGET_FILE:disvm-editor>"
      "${CMAKE_CURRENT_BINARY_DIR}/desktop-ide.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf" ${mode})
    set_tests_properties(disvm_vector_dense_selection_${mode} PROPERTIES TIMEOUT 180)
  endforeach()
endif()

# Latest-only geometry scheduling is distinct from ordered native input capture.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis"
    "${root}/ide/dis/tests/ideinklatest_test.b"
  DEPENDS limbo-bootstrap "${root}/ide/dis/tests/ideinklatest_test.b" ${module_headers} VERBATIM)
add_custom_target(disvm_inklatest_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis")
if(DVM_RUNTIME_TESTS)
  foreach(mode jit interpreter)
    add_test(NAME disvm_inklatest_${mode} COMMAND "${PYTHON_EXECUTABLE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/test_application.py" "$<TARGET_FILE:disvm>"
      "${CMAKE_CURRENT_BINARY_DIR}/ideinklatest-test.dis" ${mode} 90)
    set_tests_properties(disvm_inklatest_${mode} PROPERTIES TIMEOUT 100)
  endforeach()
endif()

# Generic copied point modifiers run without a Dis or FlashVM heap reference.
if(DVM_RUNTIME_TESTS)
  add_executable(disvm_vector_local_test test_vector_local.c vector_local.c vector_transform.c)
  target_link_libraries(disvm_vector_local_test PRIVATE m)
  add_test(NAME disvm_vector_local COMMAND disvm_vector_local_test)
endif()

# Whole managed applications, not synthetic register-only callbacks, on a Dis OS owner.
if(DVM_RUNTIME_TESTS AND DVM_RUNTIME_EDITOR)
  add_test(NAME disvm_threaded_ui_owners COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_threaded_editor.py" "$<TARGET_FILE:disvm-editor>"
    "${CMAKE_CURRENT_BINARY_DIR}/idecore-test.dis" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
  set_tests_properties(disvm_threaded_ui_owners PROPERTIES TIMEOUT 120)
endif()

# Runnable managed computation example; no synthetic replacement VM or halt exit.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/threaded-owner.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" ${includes} -o "${CMAKE_CURRENT_BINARY_DIR}/threaded-owner.dis"
    "${root}/disVM/examples/threaded-owner/main.b"
  DEPENDS limbo-bootstrap "${root}/disVM/examples/threaded-owner/main.b" ${module_headers} VERBATIM)
add_custom_target(disvm_threaded_owner_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/threaded-owner.dis")

# Real managed cyclic garbage, preserved roots, native collector joins/errors.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
   -o "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis"
   "${root}/disVM/examples/gc-worker/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-worker/main.b" VERBATIM)
add_custom_target(disvm_gc_worker_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis")
add_test(NAME disvm_native_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
 "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm>"
 "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis")
set_tests_properties(disvm_native_cycle_gc PROPERTIES TIMEOUT 180)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_idle_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis" --idle-cycles)
 set_tests_properties(disvm_idle_cycle_gc PROPERTIES TIMEOUT 130)
 set(record_cycle_overlap_module "${CMAKE_CURRENT_BINARY_DIR}/record-cycle-overlap.dis")
 add_custom_command(OUTPUT "${record_cycle_overlap_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${record_cycle_overlap_module}" "${CMAKE_CURRENT_SOURCE_DIR}/record_cycle_overlap.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/record_cycle_overlap.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_record_cycle_overlap_module ALL DEPENDS "${record_cycle_overlap_module}")
 add_test(NAME disvm_record_cycle_overlap COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${record_cycle_overlap_module}")
 set_tests_properties(disvm_record_cycle_overlap PROPERTIES TIMEOUT 330)
 set(container_cycle_overlap_module "${CMAKE_CURRENT_BINARY_DIR}/container-cycle-overlap.dis")
 add_custom_command(OUTPUT "${container_cycle_overlap_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${container_cycle_overlap_module}" "${CMAKE_CURRENT_SOURCE_DIR}/container_cycle_overlap.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/container_cycle_overlap.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_container_cycle_overlap_module ALL DEPENDS "${container_cycle_overlap_module}")
 add_test(NAME disvm_container_cycle_overlap COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${container_cycle_overlap_module}" --native-spawn --require-confirmation)
 set_tests_properties(disvm_container_cycle_overlap PROPERTIES TIMEOUT 330)
 set(large_record_cycle_module "${CMAKE_CURRENT_BINARY_DIR}/large-record-cycle.dis")
 add_custom_command(OUTPUT "${large_record_cycle_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${large_record_cycle_module}" "${CMAKE_CURRENT_SOURCE_DIR}/large_record_cycle.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/large_record_cycle.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_large_record_cycle_module ALL DEPENDS "${large_record_cycle_module}")
 add_test(NAME disvm_large_record_cycle COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${large_record_cycle_module}" --native-spawn --large-cycle)
 set_tests_properties(disvm_large_record_cycle PROPERTIES TIMEOUT 330)
 set(live_branch_charge_module "${CMAKE_CURRENT_BINARY_DIR}/live-branch-charge.dis")
 add_custom_command(OUTPUT "${live_branch_charge_module}"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -I "${u}/module"
   -o "${live_branch_charge_module}" "${CMAKE_CURRENT_SOURCE_DIR}/gc/fast/live_branch_charge.b"
  DEPENDS limbo-bootstrap "${CMAKE_CURRENT_SOURCE_DIR}/gc/fast/live_branch_charge.b" ${module_headers}
  VERBATIM)
 add_custom_target(disvm_live_branch_charge_module ALL DEPENDS "${live_branch_charge_module}")
 add_test(NAME disvm_live_branch_charge COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_record_cycle_overlap.py" "$<TARGET_FILE:disvm>"
  "${live_branch_charge_module}" --native-spawn --live-branch-charge)
 set_tests_properties(disvm_live_branch_charge PROPERTIES TIMEOUT 330)




endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_parallel_gc_graph COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/inferno_gc.py"
  --workers --flashvm-root "${root}/flashVM")
 set_tests_properties(disvm_parallel_gc_graph PROPERTIES TIMEOUT 60)
endif()
if(TARGET disvm-editor)
 add_test(NAME disvm_editor_native_cycle_gc COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runtime_gc.py" "$<TARGET_FILE:disvm-editor>"
  "${CMAKE_CURRENT_BINARY_DIR}/gc-worker.dis")
 set_tests_properties(disvm_editor_native_cycle_gc PROPERTIES TIMEOUT 180)
endif()

# Audited native callback: real managed roots remain alive outside the heap lease.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis"
  "${root}/disVM/examples/gc-native-wait/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-wait/main.b" VERBATIM)
add_custom_target(disvm_native_wait_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_wait COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_wait.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
 set_tests_properties(disvm_native_wait PROPERTIES TIMEOUT 90)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_wait COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_wait.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
  set_tests_properties(disvm_editor_native_wait PROPERTIES TIMEOUT 90)
 endif()
endif()

# Native read/readn staging, external roots, real delayed OS pipes and file handles.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis"
  "${root}/disVM/examples/gc-native-read/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-read/main.b" VERBATIM)
add_custom_target(disvm_native_read_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_read COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_read.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis")
 set_tests_properties(disvm_native_read PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_read COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_read.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-read.dis")
  set_tests_properties(disvm_editor_native_read PROPERTIES TIMEOUT 120)
 endif()
endif()

# Immutable managed-to-native output snapshots with actual pipe backpressure.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis"
  "${root}/disVM/examples/gc-native-write/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-write/main.b" VERBATIM)
add_custom_target(disvm_native_write_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_write COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_write.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis")
 set_tests_properties(disvm_native_write PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_write COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_write.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-write.dis")
  set_tests_properties(disvm_editor_native_write PROPERTIES TIMEOUT 120)
 endif()
endif()

# Formatted output releases with saved variadic roots and actual pipe backpressure.
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis"
 COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module"
  -o "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis"
  "${root}/disVM/examples/gc-native-format/main.b"
 DEPENDS limbo-bootstrap "${root}/disVM/examples/gc-native-format/main.b" VERBATIM)
add_custom_target(disvm_native_format_module ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis")
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_format COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_native_format.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis")
 set_tests_properties(disvm_native_format PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_native_format COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_native_format.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-format.dis")
  set_tests_properties(disvm_editor_native_format PROPERTIES TIMEOUT 120)
 endif()
endif()


# Actual native owners share retained instruction operands through FIFO heap turns.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_serial_heap_admission COMMAND "${PYTHON_EXECUTABLE}"
  "${root}/disVM/tests/native_serial.py" --flashvm-root "${root}/flashVM")
 set_tests_properties(disvm_serial_heap_admission PROPERTIES TIMEOUT 80 ENVIRONMENT "CC=${CMAKE_C_COMPILER}")
endif()


# Trusted local native calls exercise terminal cleanup, not shared file-call ABI.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(call_gate native_local_calls native_local_calls_control)
  set(call_control)
  if(call_gate STREQUAL "native_local_calls_control")
   set(call_control --control)
  endif()
  add_test(NAME disvm_${call_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}"
   "CPATH=${root}/disVM/host/include:${u}/Linux/arm/include"
   "NATIVE64_ARCH=${runtime_backend}"
   "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
   "${PYTHON_EXECUTABLE}" "${u}/utils/tests/native64-local-calls.py" ${call_control})
  set_tests_properties(disvm_${call_gate} PROPERTIES TIMEOUT 120)
 endforeach()
endif()


# Per-context native-call state and registration sealing, with active controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_owner_leases COMMAND "${PYTHON_EXECUTABLE}"
  "${root}/disVM/tests/native_owner.py" --flashvm-root "${root}/flashVM")
 set_tests_properties(disvm_native_owner_leases PROPERTIES TIMEOUT 100 ENVIRONMENT "CC=${CMAKE_C_COMPILER}")
endif()


# Trusted caught-call recovery: actual emu/os handlers and whole-REG borrowers.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(handler_gate native_local_handlers native_local_handlers_pending native_local_handlers_registers)
  set(handler_control)
  if(handler_gate STREQUAL "native_local_handlers_pending")
   set(handler_control --control pending)
  elseif(handler_gate STREQUAL "native_local_handlers_registers")
   set(handler_control --control registers)
  endif()
  add_test(NAME disvm_${handler_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}"
   "CPATH=${root}/disVM/host/include:${u}/Linux/arm/include"
   "NATIVE64_ARCH=${runtime_backend}"
   "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
   "${PYTHON_EXECUTABLE}" "${u}/utils/tests/native64-local-handlers.py" ${handler_control})
  set_tests_properties(disvm_${handler_gate} PROPERTIES TIMEOUT 180)
 endforeach()
endif()


# Actual additional native owners inspect the production queue during audited waits.
if(DVM_RUNTIME_TESTS)
 add_test(NAME disvm_native_queue_probe COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_queue_probe.py" "$<TARGET_FILE:disvm>"
  "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
 set_tests_properties(disvm_native_queue_probe PROPERTIES TIMEOUT 120)
 if(TARGET disvm-editor)
  add_test(NAME disvm_editor_queue_probe COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_queue_probe.py" "$<TARGET_FILE:disvm-editor>"
   "${CMAKE_CURRENT_BINARY_DIR}/native-wait.dis")
  set_tests_properties(disvm_editor_queue_probe PROPERTIES TIMEOUT 120)
 endif()
endif()


# Independent FIFO oracle and semantic native-owner admission controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_runqueue_ownership COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_runqueue.py")
 set_tests_properties(disvm_runqueue_ownership PROPERTIES TIMEOUT 60 ENVIRONMENT "CC=${CMAKE_C_COMPILER}")
endif()


# Fresh exception-message allocation must observe the recovered live caller roots.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(fresh_gate native_fresh_handlers native_fresh_handlers_publication native_fresh_handlers_map)
  set(fresh_control)
  if(fresh_gate STREQUAL "native_fresh_handlers_publication")
   set(fresh_control --control publication)
  elseif(fresh_gate STREQUAL "native_fresh_handlers_map")
   set(fresh_control --control map)
  endif()
  add_test(NAME disvm_${fresh_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}"
   "CPATH=${root}/disVM/host/include:${u}/Linux/arm/include"
   "NATIVE64_ARCH=${runtime_backend}"
   "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
   "${PYTHON_EXECUTABLE}" "${u}/utils/tests/native64-fresh-handlers.py" ${fresh_control})
  set_tests_properties(disvm_${fresh_gate} PROPERTIES TIMEOUT 240)
 endforeach()
endif()


# Real extended-up guards must remain owner-local outside serialized heap turns.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(owner_gate native_execution_owners native_execution_shared_control)
  set(owner_control)
  if(owner_gate STREQUAL "native_execution_shared_control")
   set(owner_control --control shared-execution)
  endif()
  add_test(NAME disvm_${owner_gate} COMMAND "${CMAKE_COMMAND}" -E env
   "CC=${CMAKE_C_COMPILER}" "CFLAGS=-O1 -g -fsanitize=undefined -fno-sanitize-recover=all"
   "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/native_registers.py"
   --flashvm-root "${DVM_FLASHVM_SOURCE_ROOT}" ${owner_control})
  set_tests_properties(disvm_${owner_gate} PROPERTIES TIMEOUT 60)
 endforeach()
endif()


# Private proposal checkers validate trusted metadata/IR only, never loader admission.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(local_family catalog flow state)
  if(local_family STREQUAL "catalog")
   set(local_controls normal entries tail)
  elseif(local_family STREQUAL "flow")
   set(local_controls normal branch fallthrough calls cycle depth)
  else()
   set(local_controls normal arguments binding reuse kind boolean return retire join
    canonical preparation address identity result)
  endif()
  foreach(local_control ${local_controls})
   set(local_arguments)
   if(NOT local_control STREQUAL "normal")
    set(local_arguments --control ${local_control})
   endif()
   set(local_gate disvm_dis64_private_${local_family}_${local_control})
   add_test(NAME ${local_gate} COMMAND "${CMAKE_COMMAND}" -E env
    "CC=${CMAKE_C_COMPILER}" "NATIVE64_TMPDIR=${CMAKE_CURRENT_BINARY_DIR}"
    "${PYTHON_EXECUTABLE}" "${u}/utils/tests/dis64-local-${local_family}.py" ${local_arguments})
   set_tests_properties(${local_gate} PROPERTIES TIMEOUT 180 LABELS "dis64-private")
  endforeach()
 endforeach()
endif()


# Real two-executor examples and native backpressure; no simulated dispatcher.
foreach(pool_example main rc-overlap backpressure backpressure-eight shared-file file-overlap metadata-overlap elastic elastic-stop elastic-park ui-rpc)
 if(pool_example STREQUAL "main")
  set(pool_image managed-workers)
 elseif(pool_example STREQUAL "rc-overlap")
  set(pool_image managed-rc-overlap)
 elseif(pool_example STREQUAL "backpressure")
  set(pool_image managed-backpressure)
 elseif(pool_example STREQUAL "backpressure-eight")
  set(pool_image managed-backpressure-eight)
 elseif(pool_example STREQUAL "elastic-park")
  set(pool_image managed-elastic-park)
 elseif(pool_example STREQUAL "elastic-stop")
  set(pool_image managed-elastic-stop)
 elseif(pool_example STREQUAL "elastic")
  set(pool_image managed-elastic)
 elseif(pool_example STREQUAL "metadata-overlap")
  set(pool_image managed-metadata-overlap)
 elseif(pool_example STREQUAL "file-overlap")
  set(pool_image managed-file-overlap)
 elseif(pool_example STREQUAL "shared-file")
  set(pool_image managed-shared-file)
 else()
  set(pool_image managed-ui-rpc)
 endif()
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${pool_image}.dis"
  COMMAND "$<TARGET_FILE:limbo-bootstrap>" -g -I "${u}/module" -I "${root}/ide/dis/bridge"
   -o "${CMAKE_CURRENT_BINARY_DIR}/${pool_image}.dis"
   "${root}/disVM/examples/managed-workers/${pool_example}.b"
  DEPENDS limbo-bootstrap "${root}/disVM/examples/managed-workers/${pool_example}.b"
   "${root}/ide/dis/bridge/idehost.m" VERBATIM)
 list(APPEND pool_images "${CMAKE_CURRENT_BINARY_DIR}/${pool_image}.dis")
endforeach()
add_custom_target(disvm_managed_worker_modules ALL DEPENDS ${pool_images})
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(pool_runner disvm disvm-editor)
  if(TARGET ${pool_runner})
   add_test(NAME ${pool_runner}_managed_os_executors COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_workers.py" "$<TARGET_FILE:${pool_runner}>"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-workers.dis" "${CMAKE_CURRENT_BINARY_DIR}/managed-backpressure.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-shared-file.dis" "${CMAKE_CURRENT_BINARY_DIR}/managed-ui-rpc.dis"
    "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
   set_tests_properties(${pool_runner}_managed_os_executors PROPERTIES TIMEOUT 300)
  endif()
 endforeach()
endif()


# Bounded 4/8-owner execution and eight simultaneous real native continuations.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(pool_runtime disvm disvm-editor)
  if(TARGET ${pool_runtime})
  add_test(NAME ${pool_runtime}_managed_pool_capacity COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_pool.py" "$<TARGET_FILE:${pool_runtime}>"
   "${CMAKE_CURRENT_BINARY_DIR}/managed-workers.dis"
   "${CMAKE_CURRENT_BINARY_DIR}/managed-backpressure-eight.dis"
   "$<TARGET_FILE:limbo-bootstrap>" "${u}/module"
   "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
  set_tests_properties(${pool_runtime}_managed_pool_capacity PROPERTIES TIMEOUT 300)
  endif()
 endforeach()
endif()


# Per-file FIFO uses genuine native contexts and active private removal controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_file_fifo COMMAND "${CMAKE_COMMAND}" -E env "CC=${CMAKE_C_COMPILER}"
  "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_file_gate.py")
 set_tests_properties(disvm_native_file_fifo PROPERTIES TIMEOUT 120)
endif()


# Real runtime remains live while a test-only barrier holds native file fwrite.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(file_runner disvm disvm-editor)
  if(TARGET ${file_runner})
   add_test(NAME ${file_runner}_native_file_wait COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_native_file_wait.py" "$<TARGET_FILE:${file_runner}>"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-file-overlap.dis"
    "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
   set_tests_properties(${file_runner}_native_file_wait PROPERTIES TIMEOUT 180)
  endif()
 endforeach()
endif()


# Linux/glibc-only interposition is a test facility, never a runtime prerequisite.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 include(CheckCSourceCompiles)
 check_c_source_compiles("#include <features.h>
#ifndef __GLIBC__
#error metadata fixture requires glibc
#endif
int main(void) { return 0; }" DVM_METADATA_GLIBC)
 if(DVM_METADATA_GLIBC)
  foreach(metadata_runner disvm disvm-editor)
   if(TARGET ${metadata_runner})
    add_test(NAME ${metadata_runner}_native_metadata_wait COMMAND "${PYTHON_EXECUTABLE}"
     "${CMAKE_CURRENT_SOURCE_DIR}/test_metadata_wait.py" "$<TARGET_FILE:${metadata_runner}>"
     "${CMAKE_CURRENT_BINARY_DIR}/managed-metadata-overlap.dis"
     "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
    set_tests_properties(${metadata_runner}_native_metadata_wait PROPERTIES TIMEOUT 180)
   endif()
  endforeach()
 endif()
endif()


# Actual pressure-driven growth, channel suspension and idle retirement/reaping.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(elastic_runner disvm disvm-editor)
  if(TARGET ${elastic_runner})
   add_test(NAME ${elastic_runner}_elastic_pool COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_elastic_pool.py" "$<TARGET_FILE:${elastic_runner}>"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-workers.dis"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-elastic.dis"
    "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
   set_tests_properties(${elastic_runner}_elastic_pool PROPERTIES TIMEOUT 300)
  endif()
 endforeach()
endif()


# Real helper-only allocation/release pressure while the parent process waits in I/O.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(rc_runner disvm disvm-editor)
  if(TARGET ${rc_runner})
   add_test(NAME ${rc_runner}_helper_rc COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_helper_rc.py" "$<TARGET_FILE:${rc_runner}>"
    "${CMAKE_CURRENT_BINARY_DIR}/managed-rc-overlap.dis"
    "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
   set_tests_properties(${rc_runner}_helper_rc PROPERTIES TIMEOUT 300)
  endif()
 endforeach()
endif()


# Rejected RC roles must fail before payload/allocator access; real native worlds.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_rc_roles COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_rc_roles.py")
 set_tests_properties(disvm_rc_roles PROPERTIES TIMEOUT 120)
endif()


# Typed native counter boundary/control and actual OS contention checks.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_refcounts COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${root}/disVM/tests/refcount.py")
 set_tests_properties(disvm_native_refcounts PROPERTIES TIMEOUT 180)
endif()


# Actual native object contention/parking and guard-aware TLS/world fences.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_object_guards COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${root}/disVM/tests/object_guard.py")
 set_tests_properties(disvm_native_object_guards PROPERTIES TIMEOUT 240)
endif()


# Scope cleanup must precede actual hosted native error transfer.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_owner_scopes COMMAND "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/owner_scope.py")
 add_test(NAME disvm_native_error_cleanup COMMAND "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/native_cleanup.py")
 set_tests_properties(disvm_native_owner_scopes disvm_native_error_cleanup PROPERTIES TIMEOUT 240)
endif()


# Real managed roots and asynchronous Fast retirement, plus isolated refusal roles.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 foreach(owner_runner disvm disvm-editor)
  if(TARGET ${owner_runner})
   add_test(NAME ${owner_runner}_managed_root_owners COMMAND "${PYTHON_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_owner.py" "$<TARGET_FILE:${owner_runner}>"
    "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" "${root}/ide/assets/fonts/NotoSansMono-Regular.ttf")
   set_tests_properties(${owner_runner}_managed_root_owners PROPERTIES TIMEOUT 240)
  endif()
 endforeach()
 add_test(NAME disvm_managed_owner_roles COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_owner_roles.py")
 set_tests_properties(disvm_managed_owner_roles PROPERTIES TIMEOUT 180)
endif()

# Source-slot lifetime protection is independent of parallel managed heap admission.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_owner_slots COMMAND "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/owner_slot.py")
 set_tests_properties(disvm_native_owner_slots PROPERTIES TIMEOUT 240)
endif()


# Actual opaque native registry and allocation-outside-lock semantic controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_root_registry COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_root_registry.py")
 set_tests_properties(disvm_native_root_registry PROPERTIES TIMEOUT 180)
endif()


# Concurrent native arena metadata; returning callbacks reenter after unlock.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_heap_pool COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_heap_pool.py")
 set_tests_properties(disvm_native_heap_pool PROPERTIES TIMEOUT 180)
endif()


# Actual Loader construction and emergency exception branch; private-source controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_boundary_count_owners COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_boundary_counts.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_boundary_count_owners PROPERTIES TIMEOUT 180)
endif()


# Real cyclic graph and nested stack diagnostic; explicit pre-unsafe controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_stack_check_reentry COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_stack_check.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_stack_check_reentry PROPERTIES TIMEOUT 180)
endif()


# Actual bounded retirement metadata under joined native producers/consumer.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_retirement_queue COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_retirement_queue.py")
 set_tests_properties(disvm_native_retirement_queue PROPERTIES TIMEOUT 180)
 add_test(NAME disvm_rc_reservation_rollback COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_rc_reservation.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_rc_reservation_rollback PROPERTIES TIMEOUT 180)
endif()


# Native GC scalar metadata backends; no concurrent graph admission.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_gc_fields COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_gc_fields.py")
 set_tests_properties(disvm_native_gc_fields PROPERTIES TIMEOUT 180)
endif()


# Actual SysV x64 machine owner tails, cache retirement and pre-unsafe controls.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_owner_tail_entries COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_owner_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_owner_tail_entries PROPERTIES TIMEOUT 180)
endif()


# Real mapped/raw records through checked SysV x64 aggregate entries.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_aggregate_tail_entries COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_aggregate_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_aggregate_tail_entries PROPERTIES TIMEOUT 180)
endif()


# Linearizable native pending exchange plus actual retained collector consumption.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_gc_pending_exchange COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_gc_exchange.py")
 set_tests_properties(disvm_gc_pending_exchange PROPERTIES TIMEOUT 180)
 add_test(NAME disvm_gc_pending_collector COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_gc_pending.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_gc_pending_collector PROPERTIES TIMEOUT 180)
endif()


# Canonical native sidecars and actual independent managed capture identity.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_guard_registry COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/guard_registry.py")
 set_tests_properties(disvm_native_guard_registry PROPERTIES TIMEOUT 180)
endif()


if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_managed_guard_registry COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_guards.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_managed_guard_registry PROPERTIES TIMEOUT 180)
endif()


# Keep direct capture's established link/late-accept/rollback controls in CI.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_direct_capture COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_direct_capture.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_direct_capture PROPERTIES TIMEOUT 180)
endif()


if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_cell_access COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/cell_access.py")
 set_tests_properties(disvm_native_cell_access PROPERTIES TIMEOUT 180)
 add_test(NAME disvm_shared_pointer_cells COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_pointer.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_pointer_cells PROPERTIES TIMEOUT 180)
endif()


# Native mixed range/cell exclusion and actual six-op guarded profile.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_range_access COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/range_access.py")
 set_tests_properties(disvm_native_range_access PROPERTIES TIMEOUT 180)
 if(runtime_backend STREQUAL "amd64")
  add_test(NAME disvm_shared_byte_copies COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_bytes.py" "$<TARGET_FILE:disvm>"
   "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
  add_test(NAME disvm_guarded_byte_tails COMMAND "${PYTHON_EXECUTABLE}"
   "${CMAKE_CURRENT_SOURCE_DIR}/test_bytes_tail.py" "$<TARGET_FILE:disvm>"
   "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
  set_tests_properties(disvm_shared_byte_copies disvm_guarded_byte_tails PROPERTIES TIMEOUT 180)
 endif()
endif()


# Actual native preallocated root publication and occurrence isolation.
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_root_batches COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_root_batch.py")
 set_tests_properties(disvm_native_root_batches PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_managed_root_batches COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_root_batch.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_managed_root_batches PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_mapped_copy COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_mapped.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_mapped_copy PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_guarded_mapped_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_aggregate_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --guarded-copies)
 set_tests_properties(disvm_guarded_mapped_tails PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_head_copy COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_head.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_head_copy PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_list_pointer COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_list_pointer.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_guarded_list_pointer_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_owner_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --guarded-copies)
 set_tests_properties(disvm_shared_list_pointer disvm_guarded_list_pointer_tails PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_list_bytes COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_list_bytes.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_list_bytes_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_list_bytes_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_list_bytes disvm_list_bytes_tails PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_integer COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_integer.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_integer_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_integer_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_integer disvm_integer_tails PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_arithmetic COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_arithmetic.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_arithmetic_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_arithmetic_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_arithmetic disvm_arithmetic_tails PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_small_ranges COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/small_range.py")
 set_tests_properties(disvm_native_small_ranges PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_arithmetic_zero_errors COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_arithmetic_errors.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_arithmetic_zero_errors PROPERTIES TIMEOUT 180)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_shifts COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_shift.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_shift_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shift_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_shift_zero_errors COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shift_errors.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_shifts disvm_shift_tails disvm_shift_zero_errors PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_jump_boundary COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_native_jump.py")
 set_tests_properties(disvm_native_jump_boundary PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_integer_branches COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_branch.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_integer_branch_tails COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_branch_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_integer_branches disvm_integer_branch_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_small_extension COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/small_extend.py")
 set_tests_properties(disvm_native_small_extension PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_array_length COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_array_length.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_array_length_tail COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_array_length_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_array_length disvm_array_length_tail PROPERTIES TIMEOUT 300)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
 add_test(NAME disvm_native_range_extension COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${root}/disVM/tests/range_extend.py")
 set_tests_properties(disvm_native_range_extension PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_list_length COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_list_length.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_list_length_tail COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_list_length_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_list_length disvm_list_length_tail PROPERTIES TIMEOUT 300)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_string_length COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_string_length.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_string_length_tail COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_string_length_tail.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_string_length disvm_string_length_tail PROPERTIES TIMEOUT 300)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_string_branches COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_string_branch.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_string_branch_tails COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_string_branch_tail.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_string_branches disvm_string_branch_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
 add_test(NAME disvm_shared_real_branches COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_real_branch.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 add_test(NAME disvm_real_branch_tails COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_real_branch_tail.py" "$<TARGET_FILE:disvm>" "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_shared_real_branches disvm_real_branch_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_native_root_replacement COMMAND "${CMAKE_COMMAND}" -E env
        "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_root_replace.py")
    set_tests_properties(disvm_native_root_replacement PROPERTIES TIMEOUT 240)
endif()
if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_managed_root_replacement COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_managed_root_replace.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_managed_root_replacement PROPERTIES TIMEOUT 240)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_shared_real_arithmetic COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_real_arithmetic.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_real_arithmetic_tails COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_real_arithmetic_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_shared_real_arithmetic disvm_real_arithmetic_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_guarded_dispatch_closure COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_guarded_dispatch.py" "${CMAKE_CURRENT_BINARY_DIR}/native")
    set_tests_properties(disvm_guarded_dispatch_closure PROPERTIES TIMEOUT 120)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_shared_real_conversion COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_shared_real_conversion.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_real_conversion_tails COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_real_conversion_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_shared_real_conversion disvm_real_conversion_tails PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_generic_heap_holds COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_heap_hold.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_generic_heap_holds PROPERTIES TIMEOUT 360)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING)
    add_test(NAME disvm_dedicated_spawn_portable COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_continuation.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --portable-overlap)
    set_tests_properties(disvm_dedicated_spawn_portable PROPERTIES TIMEOUT 150)
endif()

if(DVM_RUNTIME_TESTS AND CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT CMAKE_CROSSCOMPILING AND runtime_backend STREQUAL "amd64")
    add_test(NAME disvm_record_new COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_record_new.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_record_new PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_module_continuation COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_module_continuation.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_module_continuation PROPERTIES TIMEOUT 120)
    add_test(NAME disvm_channel_guest_wait COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    add_test(NAME disvm_dedicated_channel_close COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --close-only)
    add_test(NAME disvm_dedicated_cancelwait COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --cancelwait-only)
    add_test(NAME disvm_dedicated_table_cancel COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-cancel-only)
    add_test(NAME disvm_dedicated_table_fifo COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-fifo-only)
    add_test(NAME disvm_dedicated_table_kill_head COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_channel_guest_wait.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module" --table-kill-head-only)
    set_tests_properties(disvm_dedicated_table_kill_head PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_dedicated_table_fifo PROPERTIES TIMEOUT 180)
    set_tests_properties(disvm_dedicated_table_cancel PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_dedicated_cancelwait PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_dedicated_channel_close PROPERTIES TIMEOUT 90)
    set_tests_properties(disvm_channel_guest_wait PROPERTIES TIMEOUT 90)
    add_test(NAME disvm_mapped_prepare COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_prepare.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_prepare PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_mapped_cons COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_cons.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_cons PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_mapped_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_mapped_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_mapped_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_raw_cons COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_raw_cons.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_raw_cons PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_raw_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_raw_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_raw_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_scalar_cons COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_scalar_cons.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_scalar_cons PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_scalar_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_scalar_tail.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_scalar_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_value_prepare COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_value_prepare.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_value_prepare PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_pointer_transaction COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pointer_transaction.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_pointer_transaction PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_guarded_pointer_tail COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pointer_tail.py" --guarded-copies "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_guarded_pointer_tail PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_pointer_prepare COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_pointer_prepare.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_pointer_prepare PROPERTIES TIMEOUT 360)
    add_test(NAME disvm_heap_commit COMMAND "${PYTHON_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/test_heap_commit.py" "$<TARGET_FILE:disvm>"
        "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
    set_tests_properties(disvm_heap_commit PROPERTIES TIMEOUT 360)
endif()
