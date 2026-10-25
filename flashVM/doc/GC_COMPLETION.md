# Collector contracts and regression coverage

The collector traces objects, not arbitrary C memory. Roots are the global graph,
explicit pins, live AVM1 operand stacks / parameters / locals / receivers, public
[`FVM_Stack`](../src/capi/stack_c.cpp#L18) contents and captures registered by [`fvm_register_function_rooted`](../include/flashvm/value.h#L160).
Replacing a registered function removes its old capture. A capture can be a table
containing multiple objects. Native callbacks still defer automatic collection
and reject explicit reentrant collection: unregistered C locals are not roots.
Keep API stacks and root scopes within their context lifetime.

Marking is iterative and handles cycles through properties, indexes, prototypes
and metatables. Sweeping compacts the object array in linear time. Callable
callbacks now live on their objects rather than a pointer-to-unsigned-long map;
this removes stale callback entries and pointer truncation on LLP64 platforms.

Collection snapshots the original object count before invoking finalizers. Each
eligible object is finalized once. Allocations made by finalizers, and all objects
reachable from those allocations, survive this pass; their finalization is deferred
to a later pass. This bounds the current finalizer pass even if callbacks allocate
new finalizable objects. Resurrection is traced again before sweep. The first
finalizer error is returned after remaining finalizers and sweep finish. The freed
count remains meaningful even when a finalizer reports an error.

A metatable's native `__gc` callback takes precedence. Alternatively, a string
property `__gc` names an existing AVM1 function. It receives `(object, metatable,
object)` and `this` is the finalized object. These are named-function extensions;
function-valued closures are not supported by the current interpreter.

Context teardown rejects reentrant teardown and collection. Creating objects
while tearing down returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), so shutdown cannot grow an
unbounded finalizer list. Teardown finalizes once and releases internal resources;
resurrection does not postpone context destruction. Its void API cannot report
finalizer errors.

Regression tests: `gc_finalizer_snapshot` covers allocation, newborn graph
retention, finalizer errors and teardown reentry; `gc_api_stack_capture` covers
capture replacement, independent stacks and callable lifetime; `gc_script_finalizer`
covers AVM1 finalization and resurrection. Existing cycle, nested-root, AVM1 safe
point and native reentry tests remain required. This is a stop-the-world object
collector, not an incremental collector. Interned/copied strings still live until
context teardown; string reclamation and allocation-failure injection remain open.

## Additional native lifetime APIs

[`fvm_stack_register_rooted`](../include/flashvm/stack.h#L58), [`fvm_callable_create_rooted`](../include/flashvm/callable.h#L26) and
[`fvm_meta_register_rooted`](../include/flashvm/metamethod.h#L27) trace their captured tables. Global stack registrations
are unconditional roots; callable and metatable captures are conditional on the
owning object being reachable. `gc_callback_captures` verifies all three.

`flashvm/userdata.h` attaches a payload and a release callback to an ordinary
object. `__gc` runs first and may resurrect it; release is deferred until actual
reclamation. Release runs once, after tracing/resurrection decisions, while all
objects still exist. Creating objects or changing pins in release callbacks is
rejected; the callback must not resurrect or access already reclaimed objects.
Exceptions from release are caught, collection reports an error and completes.
`gc_userdata` covers resurrection, one-shot release, reentry rejection, diagnostics
and teardown. Native resource payloads are not automatically scanned; store their
object captures in traced properties.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_callable_create_rooted` | function | [include/flashvm/callable.h:26](../include/flashvm/callable.h#L26) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_meta_register_rooted` | function | [include/flashvm/metamethod.h:27](../include/flashvm/metamethod.h#L27) |
| `fvm_register_function_rooted` | function | [include/flashvm/value.h:160](../include/flashvm/value.h#L160) |
| `FVM_Stack` | struct | [src/capi/stack_c.cpp:18](../src/capi/stack_c.cpp#L18) |
| `fvm_stack_register_rooted` | function | [include/flashvm/stack.h:58](../include/flashvm/stack.h#L58) |
<!-- source-links:end -->
