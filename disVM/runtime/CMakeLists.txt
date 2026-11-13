# Copyright (c) 2026 bhgv
# EN: Native Dis configuration; compose ordered build and test modules.
# RU: Конфигурация нативного Dis; последовательное подключение сборки и тестов.
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
find_package(Threads REQUIRED)
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
# EN/RU: A hook belongs to its OS-thread owner; collectors never inherit it.
# Callback принадлежит владельцу OS-потока; сборщики его не наследуют.
set(monitor_declaration "extern\tvoid\t\t(*heapmonitor)(int, void*, ulong);")
string(FIND "${native_interp}" "${monitor_declaration}" monitor_position)
if(monitor_position LESS 0)
    message(FATAL_ERROR "Heap monitor header declaration changed")
endif()
string(REPLACE "${monitor_declaration}" "extern __thread void (*heapmonitor)(int, void *, ulong);" native_interp "${native_interp}")
string(REPLACE "\tint\tnsend;" "\tWORD\tnsend;" native_interp "${native_interp}")
string(REPLACE "\tint\tnrecv;" "\tWORD\tnrecv;" native_interp "${native_interp}")
# Route hosted C write barriers through the same scalar fields as the collector.
string(APPEND native_interp "\n#ifdef DVM_RUNTIME_GC_FIELDS\n#include \"gc_fields.h\"\n#undef Setmark\n#define Setmark(h) dvm_runtime_gc_mark(&(h)->color, &mutator, &nprop)\n#endif\n")
file(WRITE "${view}/interp.h" "${native_interp}\n#include \"disvm/native_context.h\"\nextern REG *dvm_native_registers(void);\n#define DVM_R (*dvm_native_registers())\nextern int dvm_runtime_leaf(Jit *, int, REG *);\nextern int dvm_runtime_fast(int, int (*)(REG *), REG *);\nextern unsigned long dvm_runtime_instructions;\nextern unsigned long dvm_runtime_instruction_limit;\nextern void dvm_runtime_step(void);\n")
# Source-view regeneration must never silently drop reviewed local adaptations.
file(REMOVE "${view}/prepare.stamp" "${view}/runt.h" "${view}/sysmod.h" "${view}/unsupported.c" "${view}/srv.c" "${view}/srvmod.h" "${view}/loadermod.h" "${view}/mathmod.h" "${view}/idehostmod.h")

set(host_prepare_args)
set(channel_prepare_inputs
  prepare_channel_constructors.py prepare_channel_matches.py
  prepare_alt_results.py prepare_alt_waits.py prepare_channel_queues.py
  prepare_channel_buffer.py)
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
 COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_guarded_leaves.py" --output "${view}"
 COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_owner_monitor.py" --output "${view}"
 COMMAND "${CMAKE_COMMAND}" -E touch "${view}/prepare.stamp"
    DEPENDS prepare_owner_monitor.py prepare_guarded_leaves.py ${channel_prepare_inputs} limbo-bootstrap
    prepare.py prepare_jit_publication.py jit_publication.h prepare_string_view.py
    string_view_sync.h prepare_channel_new.py prepare_record_new.py record_new.h
    prepare_mapped_cons.py mapped_cons.h prepare_raw_cons.py raw_cons.h
    prepare_scalar_cons.py scalar_cons.h "${root}/disVM/examples/native-module/nativedemo.m" ${DVM_LIMBO_BINDINGS}
    "${root}/disVM/module/bigtable.m" "${root}/disVM/module/processctl.m" ${runtime_inputs} "${root}/ide/dis/bridge/idehost.m"
    "${root}/disVM/module/fircompiler.m" native_tables.c relative_sequence.c prepare_exception.py
    prepare_type_counts.py prepare_heap_counts.py prepare_opcode_counts.py prepare_module_counts.py
    prepare_snapshots.py prepare_observations.py prepare_jit_counts.py prepare_pointer_builder.py
    prepare_pointer_tail.py prepare_owner_tail.py prepare_aggregate_tail.py prepare_shared_pointer.py
    prepare_shared_bytes.py prepare_shared_mapped.py shared_mapped.h prepare_shared_head.py
    shared_head.h prepare_shared_list_pointer.py shared_list_pointer.h prepare_shared_list_bytes.py
    shared_list_bytes.h prepare_shared_integer.py shared_integer.h prepare_shared_arithmetic.py
    shared_arithmetic.h prepare_shared_shift.py shared_shift.h prepare_shared_branch.py
    shared_branch.h prepare_shared_array_length.py shared_array_length.h prepare_shared_real_conversion.py
    shared_real_conversion.h prepare_shared_real_arithmetic.py shared_real_arithmetic.h prepare_shared_real_branch.py
    shared_real_branch.h prepare_shared_string_branch.py shared_string_branch.h prepare_shared_string_length.py
    shared_string_length.h prepare_shared_list_length.py shared_list_length.h prepare_bytes_tail.py
    shared_bytes.h bytes_fast.h shared_pointer.h aggregate_fast.h
    owner_fast.h prepare_roots.py prepare_boundary_counts.py prepare_stack_check.py
    prepare_gc_fields.py "${u}/include/interp.h" root_registry.h pointer_builder.h
    heap_snapshot.h module_counts.h heap_counts.h type_counts.h
    exception_host.h table_leases.h "${u}/emu/port/exception.c" "${root}/common/table/relative.h"
    "${u}/module/sys.m" "${u}/module/srv.m" "${u}/module/loader.m" "${u}/module/math.m"
    "${u}/emu/port/srv.c" VERBATIM)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${runtime_inputs}
  "${root}/disVM/host/prepare_native_registers.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/prepare_guarded_leaves.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/prepare_owner_monitor.py")
set(core alt conv dec disc exception gc heap link load loader math raise readmod runt stack string validstk xec comp-${runtime_backend})
set(sources)
foreach(unit IN LISTS core)
 list(APPEND sources "${view}/${unit}.c")
endforeach()
add_custom_target(disvm_runtime_sources DEPENDS "${view}/prepare.stamp")
set_property(SOURCE "${view}/gc.c" APPEND PROPERTY COMPILE_DEFINITIONS rungc=dvm_retained_rungc)
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
    runqueue_native.c
    process_lifetime.c
    owner_messages.c
    channel_match.c
    host_cancellations.c
    thread_registrar.c
    queue_probe.c
    managed_workers.c
    process_environment.c
    file_gate.c
    file_slots.c
    seek_sys.c
    metadata_wait.c
    open_wait.c
    close_worker.c
    runtime_gc.c
    runtime_rc.c
    gc/common/module_graph.c
    gc/common/graph_storage.c
    gc/common/concurrent_trace_guard.c
    gc/common/concurrent_runtime.c
    gc/common/collector_environment.c
    gc/common/native_admission.c
    gc/fast/retirement_pass.c
    gc/fast/exception_retirement.c
    gc/fast/managed_retirement.c
    gc/common/process_roots.c
    gc/fast/graph_confirmation.c
    gc/fast/packet_budget.c
    gc/fast/file_retirement.c
    gc/fast/table_retirement.c
    gc/fast/module_retirement.c
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
    module_registry.c
    module_counts_probe.c
    heap_snapshot.c
    heap_pool.c
    root_registry.c
    root_batch.c
    root_registry_probe.c
    pointer_stage.c
    record_new.c
    allocation_publication.c
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
    string_view_sync.c
    jit_publication.c
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
    managed_owner_service.c
    managed_access.c
    managed_owner_probe.c
    guard_registry_probe.c
    sleep_sys.c
    read_sys.c
    write_sys.c
    formatted_sys.c
    io_buffer.c
    clock_sys.c
    sys_capture.c
    remote_managed.c
    remote_managed_probe.c
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
set(DVM_REMOTE_TESTS ${DVM_RUNTIME_TESTS} CACHE BOOL "Native proxy lifecycle checks" FORCE)
add_subdirectory("${root}/disVM/remote" remote)
target_link_libraries(disvm PRIVATE disvm_remote_session)

# EN/RU: Production host targets stay active; historical reference registrations
# are archived. Native-only acceptance is the sole default test profile.
# Host-цели остаются; исторические reference-регистрации больше не подключаются.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/EditorTargets.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/NativeOnlyTests.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/NativeUnitTests.cmake")
