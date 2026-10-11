# Collector contracts and regression coverage

The collector traces objects, not arbitrary C memory. Roots are the global graph,
explicit pins, live AVM1 operand stacks / parameters / locals / receivers, public
`FVM_Stack` contents and captures registered by `fvm_register_function_rooted`.
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
while tearing down returns `FVM_INVALID_ARGUMENT`, so shutdown cannot grow an
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
