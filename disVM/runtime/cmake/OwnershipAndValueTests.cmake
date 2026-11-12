# Copyright (c) 2026 bhgv
# EN: Ordered OwnershipAndValueTests registrations; include from the runtime source directory.
# RU: Регистрация OwnershipAndValueTests; подключать из каталога исходников runtime.


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
 add_test(NAME disvm_native_pool_search COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_pool_search.py")
 set_tests_properties(disvm_native_pool_search PROPERTIES TIMEOUT 90)
 add_test(NAME disvm_native_root_distribution COMMAND "${CMAKE_COMMAND}" -E env
  "CC=${CMAKE_C_COMPILER}" "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_root_distribution.py")
 set_tests_properties(disvm_native_root_distribution PROPERTIES TIMEOUT 90)
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
 add_test(NAME disvm_rc_mandatory_publication COMMAND "${PYTHON_EXECUTABLE}"
  "${CMAKE_CURRENT_SOURCE_DIR}/test_rc_reservation.py" "$<TARGET_FILE:disvm>"
  "$<TARGET_FILE:limbo-bootstrap>" "${u}/module")
 set_tests_properties(disvm_rc_mandatory_publication PROPERTIES TIMEOUT 180)
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
