# FlashVM tracing garbage collector

## Model

Each [`FVM_Context`](../src/core/runtime.hpp#L245) owns its objects. A full mark-and-sweep pass begins at the VM global object and all explicitly pinned objects. It traces own named properties, numeric indexed properties, prototypes and metatables. It therefore collects **cycles** that are unreachable from roots; reference counting alone cannot do this.

The collector is not concurrent. Use it only on the context's owning thread, at a VM safe point. Never call collection recursively from `__gc` or while the AVM1 interpreter is actively using unrooted temporary objects. AVM1 frames are rooted and automatic safe points are enabled. Public stacks and registered native captures are also traced; arbitrary native C locals are not scanned. See [collector contracts](GC_COMPLETION.md).

## API

- `fvm_gc_pin(context, object)` / `fvm_gc_unpin(...)`: reference-counted external roots; pin before retaining a borrowed pointer across a collection.
- `fvm_gc_collect(context, &reclaimed)`: one full pass, even when suspended. The count includes freed objects only.
- `fvm_gc_mode(context, mode)`: sets `-100..100` or [`FVM_GC_SUSPENDED`](../include/flashvm/gc.h#L23); [`FVM_GC_FULL`](../include/flashvm/gc.h#L26) triggers one pass without changing the stored mode.
- `fvm_gc_state(context, &mode)`: returns the stored mode, with 0 meaning normal.
- `fvm_gc_step(context, &reclaimed)`: performs a pass only after an allocation threshold. Suspended mode prevents it.
- `fvm_gc_object_count(context, &count)`: diagnostic count.

The threshold is `1000 - 9*mode` allocations: at `-100` it is 1900, at 0 it is 1000, and at `100` it is 100. The count is reset after a collection. This is a simple tunable policy, not a time-based or incremental GC.

## Finalizers

Before reclaiming unreachable objects, the collector calls registered native `__gc` callbacks once per object. Each callback receives `(original_object, defining_metatable, original_object)` and the VM context. Finalizers may resurrect an object by storing it in a rooted table; the collector performs another mark pass before freeing. A resurrected object is never finalized twice. Named AVM1 finalizers are supported; see [collector contracts](GC_COMPLETION.md) for precedence, allocation and error handling.

**Important:** borrowed `FVM_Object *` pointers are invalid after their object is reclaimed. Objects reachable only from an unpinned C local variable can be collected. The global object is always rooted.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:245](../src/core/runtime.hpp#L245) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `FVM_GC_FULL` | macro / define | [include/flashvm/gc.h:26](../include/flashvm/gc.h#L26) |
| `fvm_gc_mode` | function | [include/flashvm/gc.h:37](../include/flashvm/gc.h#L37) |
| `fvm_gc_object_count` | function | [include/flashvm/gc.h:57](../include/flashvm/gc.h#L57) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_gc_state` | function | [include/flashvm/gc.h:40](../include/flashvm/gc.h#L40) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_GC_SUSPENDED` | macro / define | [include/flashvm/gc.h:23](../include/flashvm/gc.h#L23) |
| `fvm_gc_unpin` | function | [include/flashvm/gc.h:54](../include/flashvm/gc.h#L54) |
| `FVM_Object` | struct | [src/core/runtime.hpp:121](../src/core/runtime.hpp#L121) |
<!-- source-links:end -->
