# GC troubleshooting / Диагностика GC

If an object disappears unexpectedly, verify it was reachable from `global` or pinned using `fvm_gc_pin`. Avoid retaining raw `FVM_Object*` beyond a collection without pinning.

Если объект неожиданно исчезает, проверьте его достижимость из `global` или закрепление `fvm_gc_pin`. Не храните голые `FVM_Object*` через сборку без закрепления.

If memory is not reclaimed, inspect `fvm_gc_object_count`, native pin counts and cycles reachable from roots. A cycle is not garbage if a live root points into it.
