# FlashVM tracing garbage collector

## Model

Each `FVM_Context` owns its objects. A full mark-and-sweep pass begins at the VM global object and all explicitly pinned objects. It traces own named properties, numeric indexed properties, prototypes and metatables. It therefore collects **cycles** that are unreachable from roots; reference counting alone cannot do this.

The collector is not concurrent. Use it only on the context's owning thread, at a VM safe point. Never call collection recursively from `__gc` or while the AVM1 interpreter is actively using unrooted temporary objects. The current implementation does **not** automatically trace the AVM1 operand stack or native C stack; collection is deliberately explicit through `fvm_gc_step`/`fvm_gc_collect`. Future integration must root interpreter frames before enabling automatic safe points.

## API

- `fvm_gc_pin(context, object)` / `fvm_gc_unpin(...)`: reference-counted external roots; pin before retaining a borrowed pointer across a collection.
- `fvm_gc_collect(context, &reclaimed)`: one full pass, even when suspended. The count includes freed objects only.
- `fvm_gc_mode(context, mode)`: sets `-100..100` or `FVM_GC_SUSPENDED`; `FVM_GC_FULL` triggers one pass without changing the stored mode.
- `fvm_gc_state(context, &mode)`: returns the stored mode, with 0 meaning normal.
- `fvm_gc_step(context, &reclaimed)`: performs a pass only after an allocation threshold. Suspended mode prevents it.
- `fvm_gc_object_count(context, &count)`: diagnostic count.

The threshold is `1000 - 9*mode` allocations: at `-100` it is 1900, at 0 it is 1000, and at `100` it is 100. The count is reset after a collection. This is a simple tunable policy, not a time-based or incremental GC.

## Finalizers

Before reclaiming unreachable objects, the collector calls registered native `__gc` callbacks once per object. Each callback receives `(original_object, defining_metatable, original_object)` and the VM context. Finalizers may resurrect an object by storing it in a rooted table; the collector performs another mark pass before freeing. A resurrected object is never finalized twice. Script-function finalizers, userdata finalizers, automatic rooting of native stack values and a script-level `gc` object remain future work.

**Important:** borrowed `FVM_Object *` pointers are invalid after their object is reclaimed. Objects reachable only from an unpinned C local variable can be collected. The global object is always rooted.
