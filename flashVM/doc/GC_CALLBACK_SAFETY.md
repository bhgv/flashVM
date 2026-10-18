# GC safety during native callbacks (English)

FlashVM's collector is context-local and is not safe to invoke concurrently
from a second OS thread. Every C callback invoked through the native-function,
stack-function, callable-object, or native metamethod dispatch paths now runs
inside a C++98 RAII guard. While the guard is active:

- `fvm_gc_collect()` returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).
- `fvm_gc_step()` returns [`FVM_OK`](../include/flashvm/flashvm.h#L42), does not collect, and sets the optional
  reclaimed count to zero.
- Nested callbacks increment the guard depth; the outermost return restores
  the previous state even when a C++ exception is translated to an API error.
- `__gc` finalizers already run during a collection, so collection reentry is
  rejected independently of the callback guard.

The guard prevents collection *inside* a callback, but does not turn raw object
pointers into permanent roots. Retain objects with `fvm_gc_pin()` or scoped
roots (`fvm_gc_roots_begin/add/end`) if their lifetime extends beyond a call.

`fvm_gc_stats()` accepts a [`FVM_GCStats`](../include/flashvm/gc.h#L70) structure with `struct_size` set to
`sizeof(FVM_GCStats)`. It reports object counts, pending allocations, GC mode,
threshold and native callback depth. Counts are diagnostic, not byte totals.

## Limitations

This is not a complete moving, incremental or generational collector. Strings
are still owned separately by the context; not every foreign API call has a
formal root contract. Automatic AVM1 safe points and all cross-thread lifetime
paths still need verification. Avoid calling into the same context from
multiple OS threads. Do not assume a borrowed pointer survives collection.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_pin` | function | [include/flashvm/gc.h:48](../include/flashvm/gc.h#L48) |
| `fvm_gc_roots_begin` | function | [include/flashvm/gc_roots.h:27](../include/flashvm/gc_roots.h#L27) |
| `fvm_gc_stats` | function | [include/flashvm/gc.h:86](../include/flashvm/gc.h#L86) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_GCStats` | struct | [include/flashvm/gc.h:70](../include/flashvm/gc.h#L70) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
<!-- source-links:end -->
