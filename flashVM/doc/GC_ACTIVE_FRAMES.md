# AVM1 active-frame garbage-collection roots

The collector now traces the operand stack, lexical local bindings and receiver of each active AVM1 execution frame. Frames are linked to the owning [`FVM_Context`](../src/core/runtime.hpp#L257) and removed using C++98 stack unwinding on all exit paths. Nested script calls contribute separate frames.

The existing global-object and explicitly pinned-object roots remain active. Cyclic references among unreachable objects are still collected by mark-and-sweep; the finalization phase re-marks after callbacks to account for resurrection.

## Safety and limitations

This historical active-frame milestone does not certify complete GC. The current
implementation also traces API stacks and registered native captures, and the
native-call barrier defers automatic collection/rejects explicit collection
until it is safe. External C object pointers require pins or scoped roots.
Context-owned string buffers retain their lifetime until context destruction;
exact accounting/native result snapshots are described in GC_STRING_STORAGE.md.
Full string/native resource/concurrency acceptance remains open. Refer to
GC_CALLBACK_SAFETY.md and GC_AVM1_SAFE_POINTS.md for current scheduling rules.

Collection is context-local and must run on the context's owning thread. Do not call [`fvm_gc_collect`](../include/flashvm/gc.h#L54) concurrently with the same context's execution. The public C ABI is unchanged by this stage.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Context` | struct | [src/core/runtime.hpp:257](../src/core/runtime.hpp#L257) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../include/flashvm/gc.h#L54) |
<!-- source-links:end -->
