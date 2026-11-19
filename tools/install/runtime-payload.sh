# Copyright (c) 2026 bhgv
# EN: Runtime bytecode, launchers, source samples and SDK metadata.
# RU: Упаковка продуктов без исполнения целевых программ.


# EN: Borrow install_component locals (base/destination/component/source) through Bash scope;
# use shared copy/program helpers. Copy failures propagate through set -e; no owned inputs.
# RU: Заимствовать base/destination/component/source из install_component через область Bash;
# использовать общие copy/program. Ошибки передаются через set -e, входы не поглощаются.
install_runtime_payload() {
    file=$(program "$base" limbo-bootstrap)
    copy_file "$file" "$destination/limbo-native" 755
    copy_file "$base/bootstrap/target-abi.json" "$destination/ABI.json"
    for file in "$base"/*.dis "$base"/*.sbl; do
        case "$(basename "$file")" in
            managed-workers.dis | managed-workers.sbl) continue ;;
        esac
        if [[ "$native_demo_enabled" != ON && ( "$(basename "$file")" == native-demo.dis || "$(basename "$file")" == native-demo.sbl ) ]]; then
            continue
        fi
        if [[ "$compiler_enabled" != ON && ( "$(basename "$file")" == compiler-service.dis ||
            "$(basename "$file")" == compiler-service.sbl ||
            "$(basename "$file")" == compiler-service-check.dis ||
            "$(basename "$file")" == compiler-service-check.sbl ||
            "$(basename "$file")" == buildpanel-test.dis ||
            "$(basename "$file")" == buildpanel-test.sbl ||
            "$(basename "$file")" == build-panel.dis ||
            "$(basename "$file")" == build-panel.sbl ||
            "$(basename "$file")" == artifact-export.dis ||
            "$(basename "$file")" == artifact-export.sbl ||
            "$(basename "$file")" == artifact-test.dis ||
            "$(basename "$file")" == artifact-test.sbl ) ]]; then
            continue
        fi
        if [[ -f "$file" ]]; then
            copy_file "$file" "$destination/$(basename "$file")"
        fi
    done
    copy_file "$ROOT/disVM/runtime/run-console.sh" "$destination/run-console.sh" 755
    if [[ "$component" == disvm-editor ]]; then
        copy_file "$ROOT/disVM/runtime/run-ide-window.sh" "$destination/run-ide.sh" 755
        copy_file "$ROOT/disVM/runtime/run-ide.sh" "$destination/run-ide-console.sh" 755
    else
        copy_file "$ROOT/disVM/runtime/run-ide.sh" "$destination/run-ide.sh" 755
    fi
    copy_file "$ROOT/disVM/runtime/run-editor.sh" "$destination/run-editor.sh" 755
    copy_file "$ROOT/disVM/runtime/run-modules.sh" "$destination/run-modules.sh" 755
    copy_file "$ROOT/disVM/runtime/run-resident-owners.sh" "$destination/run-resident-owners.sh" 755
    # EN/RU: Retire only installer-owned launchers for the removed execution pool.
    # Удалить лишь команды установщика для удалённого пула исполнителей.
    rm -f -- "$destination/run-managed-workers.sh" "$destination/managed-workers.dis" "$destination/managed-workers.sbl"
    copy_tree "$ROOT/disVM/examples/standard-modules" "$destination/examples/standard-modules"
    copy_tree "$ROOT/disVM/examples/decimal-append" "$destination/examples/decimal-append"
    copy_tree "$ROOT/ide/dis/examples/directory-browser" "$destination/examples/directory-browser"
    copy_file "$ROOT/disVM/runtime/test_directory_browser.py" "$destination/examples/directory-browser/run.py"
    copy_file "$ROOT/ide/dis/GRAPHICAL-BROWSER.md" "$destination/GRAPHICAL-BROWSER.md"
    copy_file "$ROOT/ide/dis/GRAPHICAL-BROWSER.ru.md" "$destination/GRAPHICAL-BROWSER.ru.md"
    copy_file "$ROOT/disVM/runtime/test_graphical_browser.py" "$destination/examples/directory-browser/graphical-check.py"
    copy_file "$ROOT/disVM/runtime/test_graphical_editor.py" "$destination/examples/directory-browser/test_graphical_editor.py"
    copy_file "$ROOT/ide/dis/DIRECTORIES.md" "$destination/DIRECTORIES.md"
    copy_file "$ROOT/ide/dis/DIRECTORIES.ru.md" "$destination/DIRECTORIES.ru.md"
    copy_tree "$ROOT/disVM/examples/filesystem" "$destination/examples/filesystem"
    copy_file "$ROOT/disVM/runtime/test_filesystem.py" "$destination/examples/filesystem/run.py"
    copy_file "$ROOT/disVM/runtime/FILESYSTEM.md" "$destination/FILESYSTEM.md"
    copy_file "$ROOT/disVM/runtime/FILESYSTEM.ru.md" "$destination/FILESYSTEM.ru.md"
    copy_tree "$ROOT/disVM/examples/relative-sequences" "$destination/examples/relative-sequences"
    copy_tree "$ROOT/disVM/examples/hybrid-table" "$destination/examples/hybrid-table"
    copy_tree "$ROOT/disVM/examples/table-guards" "$destination/examples/table-guards"
    copy_tree "$ROOT/disVM/examples/table-leases" "$destination/examples/table-leases"
    copy_tree "$ROOT/common/owned" "$destination/source/owned"
    copy_tree "$ROOT/common/table" "$destination/source/table"
    copy_file "$ROOT/LICENSE" "$destination/source/owned/LICENSE"
    copy_file "$ROOT/LICENSE" "$destination/source/table/LICENSE"
    copy_file "$ROOT/disVM/PAYLOAD_GUARDS.md" "$destination/PAYLOAD_GUARDS.md"
    copy_file "$ROOT/disVM/PAYLOAD_GUARDS.ru.md" "$destination/PAYLOAD_GUARDS.ru.md"
    copy_tree "$ROOT/common/table" "$destination/source/hybrid-table-core"
    copy_file "$ROOT/LICENSE" "$destination/source/hybrid-table-core/LICENSE"
    copy_file "$ROOT/disVM/libraries/README.md" "$destination/STANDARD-MODULES.md"
    copy_file "$ROOT/disVM/MODULE_AUDIT.md" "$destination/MODULE_AUDIT.md"
    copy_file "$ROOT/disVM/MODULE_AUDIT.ru.md" "$destination/MODULE_AUDIT.ru.md"
    for file in "$base/native/libraries/"*.b; do
        copy_file "$file" "$destination/source/libraries/$(basename "$file")"
    done
    copy_tree "$ROOT/ide/dis/examples/editor-console" "$destination/examples/editor-console"
    copy_file "$ROOT/disVM/runtime/README.md" "$destination/README-FIRSTSTEPS.md"
    "${PYTHON:-python3}" "$ROOT/tools/link-installed-fir-docs.py" "$ROOT" "$destination" --runtime
    copy_tree "$ROOT/ide/dis/examples/native-console" "$destination/examples/native-console"
    copy_tree "$ROOT/disVM/examples/console-check" "$destination/examples/console-check"
    copy_tree "$ROOT/disVM/examples/resident-owners" "$destination/examples/resident-owners"
    copy_tree "$ROOT/ide/dis/examples/syntax" "$destination/examples/syntax"
    copy_tree "$ROOT/disVM/examples/gc-native-wait" "$destination/examples/gc-native-wait"
    copy_tree "$ROOT/disVM/examples/gc-native-read" "$destination/examples/gc-native-read"
    copy_tree "$ROOT/disVM/examples/gc-native-write" "$destination/examples/gc-native-write"
    copy_tree "$ROOT/disVM/examples/gc-native-format" "$destination/examples/gc-native-format"
    copy_tree "$ROOT/disVM/examples/native-owner" "$destination/examples/native-owner"
    copy_tree "$ROOT/disVM/examples/native-guard-registry" "$destination/examples/native-guard-registry"
    copy_tree "$ROOT/disVM/examples/native-cell-access" "$destination/examples/native-cell-access"
    copy_tree "$ROOT/disVM/examples/native-range-access" "$destination/examples/native-range-access"
    copy_tree "$ROOT/disVM/examples/native-root-batch" "$destination/examples/native-root-batch"
    copy_tree "$ROOT/disVM/examples/guarded-mapped-copy" "$destination/examples/guarded-mapped-copy"
    copy_tree "$ROOT/disVM/examples/guarded-list-head" "$destination/examples/guarded-list-head"
    copy_tree "$ROOT/disVM/examples/guarded-pointer-list" "$destination/examples/guarded-pointer-list"
    copy_tree "$ROOT/disVM/examples/guarded-list-bytes" "$destination/examples/guarded-list-bytes"
    copy_tree "$ROOT/disVM/examples/guarded-integer" "$destination/examples/guarded-integer"
    copy_tree "$ROOT/disVM/examples/guarded-arithmetic" "$destination/examples/guarded-arithmetic"
    copy_tree "$ROOT/disVM/examples/native-small-range" "$destination/examples/native-small-range"
    copy_tree "$ROOT/disVM/examples/guarded-product-division" "$destination/examples/guarded-product-division"
    # Complete public header closure for the native-owner C tutorial.
    for native_owner_header in native_context.h native_world.h native_owner.h native_process.h; do
        copy_file "$ROOT/disVM/include/disvm/$native_owner_header" "$destination/include/disvm/$native_owner_header"
    done
    copy_file "$ROOT/flashVM/ports/thread_port.h" "$destination/source/ports/thread_port.h"
    copy_file "$ROOT/flashVM/ports/channel_sync.h" "$destination/source/ports/channel_sync.h"
    copy_file "$ROOT/flashVM/ports/linux/thread.c" "$destination/source/ports/linux/thread.c"
    copy_file "$ROOT/flashVM/ports/linux/channel_sync.c" "$destination/source/ports/linux/channel_sync.c"
    for guard_source in \
        object_guard.c guard_registry.c cell_access.c cell_access_private.h \
        range_access.c small_range.c owner_scope.c owner_slot.c \
        native_context.c native_owner.c native_world.c native_owner_private.h \
        world_request.h; do
        copy_file "$ROOT/disVM/host/$guard_source" "$destination/source/host/$guard_source"
    done
    copy_file "$ROOT/disVM/include/disvm/managed_reference.h" "$destination/include/disvm/managed_reference.h"
    copy_file "$ROOT/disVM/include/disvm/native_cleanup.h" "$destination/include/disvm/native_cleanup.h"
    copy_file "$ROOT/disVM/include/disvm/owner_scope.h" "$destination/include/disvm/owner_scope.h"
    copy_file "$ROOT/disVM/include/disvm/owner_slot.h" "$destination/include/disvm/owner_slot.h"
    copy_file "$ROOT/disVM/tests/owner_scope.c" "$destination/source/tests/owner_scope.c"
    copy_file "$ROOT/disVM/tests/owner_slot.c" "$destination/source/tests/owner_slot.c"
    copy_file "$ROOT/disVM/tests/owner_mailbox.c" "$destination/source/tests/owner_mailbox.c"
    copy_file "$ROOT/disVM/tests/owner_slot.py" "$destination/source/tests/owner_slot.py"
    copy_file "$ROOT/disVM/tests/owner_scope.py" "$destination/source/tests/owner_scope.py"
    copy_tree "$ROOT/disVM/examples/managed-owner" "$destination/examples/managed-owner"
    copy_tree "$ROOT/disVM/examples/native-owner-slot" "$destination/examples/native-owner-slot"
    copy_file "$ROOT/disVM/runtime/OWNER-SLOTS.md" "$destination/OWNER-SLOTS.md"
    copy_file "$ROOT/disVM/runtime/MANAGED-OWNERS.md" "$destination/MANAGED-OWNERS.md"
    copy_file "$ROOT/disVM/runtime/NATIVE-UNWIND.md" "$destination/NATIVE-UNWIND.md"
    copy_file "$ROOT/disVM/runtime/LOCAL-OWNERSHIP.ru.md" "$destination/LOCAL-OWNERSHIP.ru.md"
    copy_file "$ROOT/disVM/runtime/HEAP-PARALLELISM-STATUS.ru.md" "$destination/HEAP-PARALLELISM-STATUS.ru.md"
    copy_file "$ROOT/disVM/include/disvm/range_access.h" "$destination/include/disvm/range_access.h"
    copy_file "$ROOT/disVM/include/disvm/small_range.h" "$destination/include/disvm/small_range.h"
    copy_file "$ROOT/disVM/tests/range_access.c" "$destination/source/tests/range_access.c"
    copy_file "$ROOT/disVM/tests/range_access.py" "$destination/source/tests/range_access.py"
    copy_file "$ROOT/disVM/tests/small_range.py" "$destination/source/tests/small_range.py"
    copy_file "$ROOT/disVM/tests/small_extend.c" "$destination/source/tests/small_extend.c"
    copy_file "$ROOT/disVM/tests/small_extend.py" "$destination/source/tests/small_extend.py"
    copy_file "$ROOT/disVM/tests/range_extend.c" "$destination/source/tests/range_extend.c"
    copy_file "$ROOT/disVM/tests/range_extend.py" "$destination/source/tests/range_extend.py"
    copy_file "$ROOT/disVM/include/disvm/cell_access.h" "$destination/include/disvm/cell_access.h"
    copy_file "$ROOT/disVM/tests/cell_access.c" "$destination/source/tests/cell_access.c"
    copy_file "$ROOT/disVM/tests/cell_access.py" "$destination/source/tests/cell_access.py"
    copy_file "$ROOT/disVM/include/disvm/guard_registry.h" "$destination/include/disvm/guard_registry.h"
    copy_file "$ROOT/disVM/include/disvm/object_guard.h" "$destination/include/disvm/object_guard.h"
    copy_file "$ROOT/disVM/tests/guard_registry.c" "$destination/source/tests/guard_registry.c"
    copy_file "$ROOT/disVM/tests/guard_registry_faults.c" "$destination/source/tests/guard_registry_faults.c"
    copy_file "$ROOT/disVM/tests/guard_registry.py" "$destination/source/tests/guard_registry.py"
    copy_file "$ROOT/disVM/tests/object_guard.c" "$destination/source/tests/object_guard.c"
    copy_file "$ROOT/disVM/tests/object_guard.py" "$destination/source/tests/object_guard.py"
    copy_tree "$ROOT/disVM/examples/native-object-guard" "$destination/examples/native-object-guard"
    copy_file "$ROOT/disVM/host/refcount.c" "$destination/source/host/refcount.c"
    copy_file "$ROOT/disVM/include/disvm/refcount.h" "$destination/include/disvm/refcount.h"
    copy_file "$ROOT/disVM/tests/refcount.c" "$destination/source/tests/refcount.c"
    copy_file "$ROOT/disVM/tests/refcount.py" "$destination/source/tests/refcount.py"
    copy_tree "$ROOT/disVM/examples/native-refcount" "$destination/examples/native-refcount"
    copy_file "$ROOT/disVM/runtime/OBJECT-GUARDS.md" "$destination/OBJECT-GUARDS.md"
    copy_file "$ROOT/disVM/runtime/REFCOUNT-CONVERSION.md" "$destination/REFCOUNT-CONVERSION.md"
    copy_file "$ROOT/disVM/runtime/TYPE-COUNTS.md" "$destination/TYPE-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/HEAP-COUNTS.md" "$destination/HEAP-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/OPCODE-COUNTS.md" "$destination/OPCODE-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/MODULE-COUNTS.md" "$destination/MODULE-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/HEAP-SNAPSHOTS.md" "$destination/HEAP-SNAPSHOTS.md"
    copy_file "$ROOT/disVM/runtime/CALL-COUNTS.md" "$destination/CALL-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/SERVICE-COUNTS.md" "$destination/SERVICE-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/JIT-COUNTS.md" "$destination/JIT-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/POINTER-BUILDER.md" "$destination/POINTER-BUILDER.md"
    copy_file "$ROOT/disVM/runtime/POINTER-TAIL.md" "$destination/POINTER-TAIL.md"
    copy_file "$ROOT/disVM/runtime/ROOT-REGISTRY.md" "$destination/ROOT-REGISTRY.md"
    copy_file "$ROOT/disVM/runtime/HEAP-POOL.md" "$destination/HEAP-POOL.md"
    copy_file "$ROOT/disVM/runtime/BOUNDARY-COUNTS.md" "$destination/BOUNDARY-COUNTS.md"
    copy_file "$ROOT/disVM/runtime/STACK-CHECK.md" "$destination/STACK-CHECK.md"
    copy_file "$ROOT/disVM/runtime/RETIREMENT-QUEUE.md" "$destination/RETIREMENT-QUEUE.md"
    copy_file "$ROOT/disVM/runtime/GC-FIELDS.md" "$destination/GC-FIELDS.md"
    copy_file "$ROOT/disVM/runtime/OWNER-TAIL.md" "$destination/OWNER-TAIL.md"
    copy_file "$ROOT/disVM/runtime/SHARED-BYTES.md" "$destination/SHARED-BYTES.md"
    copy_file "$ROOT/disVM/runtime/ROOT-BATCH.md" "$destination/ROOT-BATCH.md"
    copy_file "$ROOT/disVM/runtime/SHARED-MAPPED.md" "$destination/SHARED-MAPPED.md"
    copy_file "$ROOT/disVM/runtime/SHARED-HEAD.md" "$destination/SHARED-HEAD.md"
    copy_file "$ROOT/disVM/runtime/SHARED-LIST-POINTER.md" "$destination/SHARED-LIST-POINTER.md"
    copy_file "$ROOT/disVM/runtime/SHARED-LIST-BYTES.md" "$destination/SHARED-LIST-BYTES.md"
    copy_file "$ROOT/disVM/runtime/SHARED-INTEGER.md" "$destination/SHARED-INTEGER.md"
    copy_file "$ROOT/disVM/runtime/SHARED-ARITHMETIC.md" "$destination/SHARED-ARITHMETIC.md"
    copy_file "$ROOT/disVM/runtime/SMALL-RANGES.md" "$destination/SMALL-RANGES.md"
    copy_file "$ROOT/disVM/runtime/PRODUCT-DIVISION.md" "$destination/PRODUCT-DIVISION.md"
    copy_file "$ROOT/disVM/runtime/SHARED-SHIFTS.md" "$destination/SHARED-SHIFTS.md"
    copy_file "$ROOT/disVM/runtime/SHARED-BRANCHES.md" "$destination/SHARED-BRANCHES.md"
    copy_file "$ROOT/disVM/runtime/ARRAY-LENGTH.md" "$destination/ARRAY-LENGTH.md"
    copy_file "$ROOT/disVM/runtime/LIST-LENGTH.md" "$destination/LIST-LENGTH.md"
    copy_file "$ROOT/disVM/runtime/STRING-LENGTH.md" "$destination/STRING-LENGTH.md"
    copy_file "$ROOT/disVM/runtime/STRING-BRANCHES.md" "$destination/STRING-BRANCHES.md"
    copy_file "$ROOT/disVM/runtime/REAL-BRANCHES.md" "$destination/REAL-BRANCHES.md"
    copy_tree "$ROOT/disVM/examples/real-arithmetic" "$destination/examples/real-arithmetic"
    if [[ -f "$base/native/amd64-emit.h" && -f "$base/native/xec.c" ]]; then
        copy_file "$base/native/amd64-emit.h" "$destination/source/dispatch/amd64-emit.h"
        copy_file "$base/native/xec.c" "$destination/source/dispatch/xec.c"
    fi
    copy_file "$ROOT/disVM/runtime/reports/guarded-dispatch-runtime110.json" "$destination/reports/guarded-dispatch-runtime110.json"
    copy_file "$ROOT/disVM/runtime/reports/guarded-dispatch-runtime109.json" "$destination/reports/guarded-dispatch-runtime109.json"
    copy_file "$ROOT/disVM/upstream/libinterp/xec.c" "$destination/source/conversion-oracle/xec.c"
    copy_tree "$ROOT/disVM/examples/heap-hold" "$destination/examples/heap-hold"
    copy_file "$ROOT/disVM/runtime/MAPPED-CONS.md" "$destination/MAPPED-CONS.md"
    copy_file "$ROOT/disVM/runtime/RAW-CONS.md" "$destination/RAW-CONS.md"
    copy_file "$ROOT/disVM/runtime/SCALAR-CONS.md" "$destination/SCALAR-CONS.md"
    copy_file "$ROOT/disVM/runtime/VALUE-PREPARE.md" "$destination/VALUE-PREPARE.md"
    copy_file "$ROOT/disVM/runtime/POINTER-TRANSACTION.md" "$destination/POINTER-TRANSACTION.md"
    copy_file "$ROOT/disVM/runtime/performance/pointer-transaction-runtime114-initial.json" "$destination/performance/pointer-transaction-runtime114-initial.json"
    copy_file "$ROOT/disVM/runtime/performance/pointer-transaction-runtime114.json" "$destination/performance/pointer-transaction-runtime114.json"
    copy_file "$ROOT/disVM/runtime/POINTER-PREPARE.md" "$destination/POINTER-PREPARE.md"
    copy_file "$ROOT/disVM/runtime/HEAP-COMMIT.md" "$destination/HEAP-COMMIT.md"
    copy_tree "$ROOT/disVM/examples/heap-commit" "$destination/examples/heap-commit"
    copy_file "$ROOT/disVM/runtime/HEAP-HOLD.md" "$destination/HEAP-HOLD.md"
    copy_file "$ROOT/disVM/runtime/REAL-CONVERSION.md" "$destination/REAL-CONVERSION.md"
    copy_tree "$ROOT/disVM/examples/real-conversion" "$destination/examples/real-conversion"
    copy_file "$ROOT/disVM/runtime/GUARDED-DISPATCH.md" "$destination/GUARDED-DISPATCH.md"
    copy_file "$ROOT/disVM/runtime/REAL-ARITHMETIC.md" "$destination/REAL-ARITHMETIC.md"
    copy_file "$ROOT/disVM/runtime/ROOT-REPLACEMENT.md" "$destination/ROOT-REPLACEMENT.md"
    copy_tree "$ROOT/disVM/examples/root-group-replace" "$destination/examples/root-group-replace"
    copy_tree "$ROOT/disVM/examples/guarded-real-branches" "$destination/examples/guarded-real-branches"
    copy_tree "$ROOT/disVM/examples/guarded-string-branches" "$destination/examples/guarded-string-branches"
    copy_tree "$ROOT/disVM/examples/guarded-string-length" "$destination/examples/guarded-string-length"
    copy_tree "$ROOT/disVM/examples/guarded-list-length" "$destination/examples/guarded-list-length"
    copy_tree "$ROOT/disVM/examples/native-range-extension" "$destination/examples/native-range-extension"
    copy_tree "$ROOT/disVM/examples/guarded-array-length" "$destination/examples/guarded-array-length"
    copy_tree "$ROOT/disVM/examples/native-small-extension" "$destination/examples/native-small-extension"
    copy_tree "$ROOT/disVM/examples/guarded-branches" "$destination/examples/guarded-branches"
    copy_tree "$ROOT/disVM/examples/guarded-shifts" "$destination/examples/guarded-shifts"
    copy_file "$ROOT/disVM/runtime/PERFORMANCE-COSTS.md" "$destination/PERFORMANCE-COSTS.md"
    copy_tree "$ROOT/disVM/runtime/performance" "$destination/performance"
    copy_file "$ROOT/disVM/runtime/SHARED-POINTER.md" "$destination/SHARED-POINTER.md"
    copy_file "$ROOT/disVM/runtime/GUARD-REGISTRY.md" "$destination/GUARD-REGISTRY.md"
    copy_file "$ROOT/disVM/runtime/GC-PENDING.md" "$destination/GC-PENDING.md"
    copy_file "$ROOT/disVM/runtime/AGGREGATE-TAIL.md" "$destination/AGGREGATE-TAIL.md"
    copy_file "$ROOT/disVM/host/include/lib9.h" "$destination/source/host/include/lib9.h"
    copy_file "$ROOT/disVM/upstream/Linux/arm/include/lib9.h" "$destination/source/upstream/Linux/arm/include/lib9.h"
    copy_file "$ROOT/disVM/upstream/include/arm-fp.h" "$destination/source/upstream/include/arm-fp.h"
    copy_file "$ROOT/disVM/upstream/include/pool.h" "$destination/source/upstream/include/pool.h"
    copy_file "$ROOT/disVM/runtime/THREADING.md" "$destination/THREADING.md"
    copy_file "$ROOT/disVM/runtime/THREADING.ru.md" "$destination/THREADING.ru.md"
    copy_file "$ROOT/disVM/runtime/SHARED-HEAP-CONCURRENCY.ru.md" "$destination/SHARED-HEAP-CONCURRENCY.ru.md"
    copy_file "$ROOT/disVM/runtime/MULTITHREADING-ROADMAP.ru.md" "$destination/MULTITHREADING-ROADMAP.ru.md"
    copy_file "$ROOT/disVM/runtime/RUNQUEUE.md" "$destination/RUNQUEUE.md"
    for threading_file in \
        record_new.c record_new.h prepare_record_new.py test_record_new.c \
        test_record_new.py mapped_prepare.c mapped_prepare.h mapped_cons.c \
        mapped_cons.h prepare_mapped_cons.py test_mapped_prepare.c test_mapped_prepare.py \
        test_mapped_cons.c test_mapped_cons.py test_mapped_tail.py raw_cons.c \
        raw_cons.h prepare_raw_cons.py test_raw_cons.c test_raw_cons.py \
        test_raw_tail.py scalar_cons.c scalar_cons.h scalar_tail.c \
        prepare_scalar_cons.py test_scalar_cons.c test_scalar_cons.py test_scalar_tail.py \
        value_prepare.c value_prepare.h test_value_prepare.c test_value_prepare.py \
        pointer_transaction.c pointer_transaction.h test_pointer_transaction.c test_pointer_transaction.py \
        benchmark_pointer_transaction.py pointer_stage.c pointer_stage.h pointer_prepare.c \
        pointer_prepare.h test_pointer_prepare.py benchmark_heap_commit.py heap_commit.c \
        heap_commit.h test_heap_commit.py heap_hold.c heap_hold.h \
        test_heap_hold.py benchmark_heap_hold.py shared_real_conversion.c shared_real_conversion.h \
        real_conversion_value.c real_conversion_value.h prepare_shared_real_conversion.py test_shared_real_conversion.py \
        test_real_conversion_tail.py benchmark_real_conversion.py test_guarded_dispatch.py benchmark_real_dispatch.py \
        shared_real_arithmetic.c shared_real_arithmetic.h real_arithmetic_value.c real_arithmetic_value.h \
        prepare_shared_real_arithmetic.py test_shared_real_arithmetic.py test_real_arithmetic_tail.py benchmark_real_arithmetic.py \
        test_root_replace.c test_root_replace.py test_managed_root_replace.py benchmark_root_replace.py \
        shared_real_branch.c shared_real_branch.h real_branch_value.c real_branch_value.h \
        prepare_shared_real_branch.py test_shared_real_branch.py test_real_branch_tail.py benchmark_real_branch.py \
        shared_string_branch.c shared_string_branch.h string_branch_value.c string_branch_value.h \
        prepare_shared_string_branch.py test_shared_string_branch.py test_string_branch_tail.py benchmark_string_branch.py \
        shared_string_length.c shared_string_length.h string_length_value.c string_length_value.h \
        prepare_shared_string_length.py test_shared_string_length.py test_string_length_tail.py benchmark_string_length.py \
        shared_list_length.c shared_list_length.h list_length_value.c list_length_value.h \
        prepare_shared_list_length.py test_shared_list_length.py test_list_length_tail.py benchmark_list_length.py \
        shared_array_length.c shared_array_length.h array_length_value.c array_length_value.h \
        prepare_shared_array_length.py test_shared_array_length.py test_array_length_tail.py benchmark_array_length.py \
        shared_branch.c shared_branch.h branch_value.c branch_value.h \
        prepare_shared_branch.py test_shared_branch.py test_branch_tail.py benchmark_shared_branch.py \
        native_jump.c native_jump.h test_native_jump.py shared_shift.c \
        shared_shift.h shift_value.c shift_value.h prepare_shared_shift.py \
        test_shared_shift.py test_shift_tail.py test_shift_errors.py benchmark_shared_shift.py \
        benchmark_product_division.py test_arithmetic_errors.py benchmark_small_arithmetic.py shared_arithmetic.c \
        shared_arithmetic.h arithmetic_value.c arithmetic_value.h prepare_shared_arithmetic.py \
        test_shared_arithmetic.py test_arithmetic_tail.py benchmark_shared_arithmetic.py shared_integer.c \
        shared_integer.h integer_value.c integer_value.h prepare_shared_integer.py \
        test_shared_integer.py test_integer_tail.py benchmark_shared_integer.py shared_list_bytes.c \
        shared_list_bytes.h prepare_shared_list_bytes.py test_shared_list_bytes.py test_list_bytes_tail.py \
        benchmark_shared_list_bytes.py shared_list_pointer.c shared_list_pointer.h prepare_shared_list_pointer.py \
        test_shared_list_pointer.py benchmark_shared_list_pointer.py list_hold.c list_hold.h \
        shared_head.c shared_head.h prepare_shared_head.py test_shared_head.py \
        benchmark_shared_head.py shared_mapped.c shared_mapped.h prepare_shared_mapped.py \
        test_shared_mapped.py benchmark_shared_mapped.py shared_access.c shared_access.h \
        shared_bytes.c shared_bytes.h byte_transfer.c byte_transfer.h \
        bytes_fast.c bytes_fast.h prepare_shared_bytes.py prepare_bytes_tail.py \
        test_shared_bytes.py test_bytes_tail.py benchmark_guarded_copies.py shared_pointer.c \
        shared_pointer.h prepare_shared_pointer.py test_shared_pointer.py benchmark_shared_pointer.py \
        managed_owner.c managed_owner.h managed_owner_probe.c managed_owner_probe.h \
        guard_registry_probe.c test_managed_guards.py runtime_gc.c runtime_gc.h \
        runtime_rc.c runtime_rc.h gc_fields.c gc_fields.h \
        prepare_gc_fields.py test_gc_fields.c test_gc_fields.py test_gc_exchange.c \
        test_gc_exchange.py test_gc_pending.py retirement_queue.c retirement_queue.h \
        test_retirement_queue.c test_retirement_queue.py test_rc_reservation.py test_rc_reservation.c \
        type_counts.c type_counts.h type_counts_probe.c heap_counts.c \
        heap_counts.h heap_counts_probe.c opcode_counts_probe.c module_counts.c \
        module_counts.h module_registry.c module_registry.h test_module_registry.c \
        test_module_registry.py test_resident_load.py test_resident_decimal.py test_resident_errors.py \
        test_native_acceptance.c test_resident_native_count.py test_gc_requests.c test_gc_requests.py \
        test_mapped_accounting.c test_mapped_accounting.py string_view_sync.c string_view_sync.h \
        prepare_string_view.py test_string_view.c test_string_view.py jit_publication.c \
        jit_publication.h prepare_jit_publication.py test_jit_publish.c test_jit_publish.py \
        test_module_kill.c test_process_roots.c test_process_roots.py test_resident_root_probe.py \
        test_load_kill.c test_load_kill.py module_counts_probe.c heap_snapshot.c \
        heap_snapshot.h heap_snapshot_probe.c heap_pool.c heap_pool.h \
        test_heap_pool.c test_heap_pool.py root_registry.c root_registry.h \
        root_registry_private.h root_batch.c root_batch.h test_root_batch.c \
        test_root_batch.py test_managed_root_batch.py root_registry_probe.c prepare_stack_check.py \
        test_stack_check.py prepare_boundary_counts.py test_boundary_counts.py prepare_roots.py \
        test_root_registry.c test_root_registry.py aggregate_fast.c aggregate_fast.h \
        prepare_aggregate_tail.py test_aggregate_tail.py owner_fast.c owner_fast.h \
        prepare_owner_tail.py test_owner_tail.py pointer_builder.c pointer_builder.h \
        pointer_builder_probe.c prepare_pointer_builder.py prepare_pointer_tail.py test_pointer_builder.py \
        test_pointer_tail.py service_counts.h service_counts_probe.c test_service_counts.py \
        prepare_snapshots.py prepare_observations.py prepare_jit_counts.py test_jit_counts.py \
        test_direct_capture.py test_mcall_counts.py test_heap_snapshot.py relink_private.py \
        prepare_module_counts.py prepare_opcode_counts.py prepare_heap_counts.py prepare_type_counts.py \
        test_type_counts.py sleep_sys.c read_sys.c write_sys.c \
        formatted_sys.c formatted_sys.h io_buffer.c io_buffer.h \
        benchmark_gc_poll.py test_native_read.py test_native_write.py test_native_format.py \
        test_native_wait.py test_queue_probe.py runqueue.c runqueue.h \
        queue_probe.c queue_probe.h managed_workers.c managed_workers.h \
        process_environment.c process_environment.h file_gate.c file_gate.h \
        file_slots.c file_slots.h seek_sys.c metadata_wait.c \
        metadata_wait.h open_wait.c open_wait.h close_worker.c \
        close_worker.h test_metadata_wait.c test_metadata_wait.py test_file_gate.c \
        test_file_gate.py test_native_file_wait.c test_native_file_wait.py test_helper_rc.py \
        test_rc_roles.c test_rc_roles.py test_managed_start.c test_resident_startup.py \
        test_removed_pool.py test_owner_messages.c owner_messages.c owner_messages.h \
        host_cancellations.c host_cancellations.h test_host_cancellations.c thread_registrar.c \
        thread_registrar.h channel_match.c channel_match.h test_resident_alt.py \
        test_resident_processctl.py test_parent_reply_exit.c test_channel_gc_snapshot.c test_file_gate_churn.c \
        test_file_gate_churn.py test_resident_files.py test_file_slot_barrier.c test_native_cancel.c \
        test_native_cancel.py test_native_kill.c test_native_kill.py test_resident_tables.py \
        test_table_preflight.c test_table_retirement.c test_table_retirement.py test_table_scope_kill.c \
        test_table_scope_kill.py test_table_transfer.py test_table_fifo_edit.py test_table_parent_exit.c \
        test_table_parent_exit.py test_close_backlog.c test_close_backlog.py bigtable_owner.h \
        gc/fast/file_retirement.c gc/fast/file_retirement.h gc/fast/table_retirement.c gc/fast/table_retirement.h; do
        copy_file "$ROOT/disVM/runtime/$threading_file" "$destination/source/threading/$threading_file"
    done
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-alt" "$destination/source/threading/fixtures/resident-alt"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-processctl" "$destination/source/threading/fixtures/resident-processctl"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-files" "$destination/source/threading/fixtures/resident-files"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-close-backlog" "$destination/source/threading/fixtures/resident-close-backlog"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-load" "$destination/source/threading/fixtures/resident-load"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-decimal" "$destination/source/threading/fixtures/resident-decimal"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-errors" "$destination/source/threading/fixtures/resident-errors"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-native-count" "$destination/source/threading/fixtures/resident-native-count"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-gc-requests" "$destination/source/threading/fixtures/resident-gc-requests"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-mapped-count" "$destination/source/threading/fixtures/resident-mapped-count"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-string-view" "$destination/source/threading/fixtures/resident-string-view"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-jit-publish" "$destination/source/threading/fixtures/resident-jit-publish"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-root-probe" "$destination/source/threading/fixtures/resident-root-probe"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-native-cancel" "$destination/source/threading/fixtures/resident-native-cancel"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-native-kill" "$destination/source/threading/fixtures/resident-native-kill"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-tables" "$destination/source/threading/fixtures/resident-tables"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-table-retirement" "$destination/source/threading/fixtures/resident-table-retirement"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-table-scope-kill" "$destination/source/threading/fixtures/resident-table-scope-kill"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-table-transfer" "$destination/source/threading/fixtures/resident-table-transfer"
    copy_tree "$ROOT/disVM/runtime/fixtures/resident-table-fifo-edit" "$destination/source/threading/fixtures/resident-table-fifo-edit"
    copy_tree "$ROOT/ide/dis/examples/workspace" "$destination/examples/workspace"
    copy_tree "$ROOT/tools/limbo-bootstrap/upstream/module" "$destination/include/limbo"
    copy_file "$ROOT/disVM/upstream/module/readdir.m" "$destination/include/limbo/readdir.m"
    copy_file "$ROOT/disVM/upstream/module/regex.m" "$destination/include/limbo/regex.m"
    copy_file "$ROOT/disVM/upstream/appl/lib/tests/regex-unicode.b" "$destination/examples/regex-unicode.b"
    copy_file "$ROOT/ide/dis/idecore.m" "$destination/include/limbo/idecore.m"
    copy_file "$ROOT/disVM/module/uibridge.m" "$destination/include/limbo/uibridge.m"
    copy_file "$ROOT/disVM/module/unibus.m" "$destination/include/limbo/unibus.m"
    copy_file "$ROOT/disVM/module/bigtable.m" "$destination/include/limbo/bigtable.m"
    copy_file "$ROOT/disVM/module/processctl.m" "$destination/include/limbo/processctl.m"
    copy_file "$ROOT/disVM/broker/brokerdiag.m" "$destination/include/limbo/brokerdiag.m"
    for file in broker.h codec.h flash_channel.h; do
        copy_file "$ROOT/disVM/broker/$file" "$destination/include/disvm/$file"
    done
    copy_tree "$ROOT/disVM/broker" "$destination/examples/broker"
    copy_tree "$ROOT/ide/dis/examples/projection" "$destination/examples/projection"
    copy_tree "$ROOT/ide/dis/examples/document-projection" "$destination/examples/document-projection"
    copy_file "$ROOT/ide/as/projection-values.as" "$destination/as/projection-values.as"
    for file in idevalue ideprojection ideinspect idedocprojection idebrowser; do
        copy_file "$ROOT/ide/dis/bridge/$file.b" "$destination/source/$file.b"
    done
    copy_file "$ROOT/ide/dis/PROJECTIONS.md" "$destination/PROJECTIONS.md"
    copy_file "$ROOT/ide/dis/VALUES.md" "$destination/VALUES.md"
    copy_file "$ROOT/ide/dis/VALUES.ru.md" "$destination/VALUES.ru.md"
    copy_file "$ROOT/ide/dis/LIVE-PROJECTIONS.md" "$destination/LIVE-PROJECTIONS.md"
    copy_file "$ROOT/ide/dis/LIVE-PROJECTIONS.ru.md" "$destination/LIVE-PROJECTIONS.ru.md"
    copy_file "$ROOT/ide/dis/WINDOWS-GUTTER.md" "$destination/WINDOWS-GUTTER.md"
    copy_file "$ROOT/ide/dis/WINDOWS-GUTTER.ru.md" "$destination/WINDOWS-GUTTER.ru.md"
    copy_file "$ROOT/ide/dis/bridge/idefiles.README.md" "$destination/FILE-WINDOWS.md"
    copy_file "$ROOT/ide/dis/bridge/idefiles.b" "$destination/source/idefiles.b"
    for file in "$ROOT/ide/dis/bridge/"*.m; do
        copy_file "$file" "$destination/include/limbo/$(basename "$file")"
    done
    "${PYTHON:-python3}" "$ROOT/tools/link-installed-runtime-docs.py" "$ROOT" "$destination"
}
