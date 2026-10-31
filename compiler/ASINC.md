# `asinc`: frame-independent function/block proposal and syntax checkpoint

[Русский](ASINC.ru.md) · [Frame-free API](../flashVM/doc/ASYNC.md) ·
[Detached rendering](../flashVM/doc/RENDER_ASYNC.md)

The requested spelling is **asinc**. The Coco/R AS1/AS2/AS3 frontends recognize:

```actionscript
var count = 0;
asinc function update(value) { count = value; }
asinc { update(count + 1); }
var task = asinc { trace(count); };
var work = asinc function(value) { return value + count; };
```

The prefix is contextual: ordinary variables, calls, property names and functions
named `asinc` still parse. AS3 namespace attributes retain their ordinary syntax.
Comments/trivia may separate the prefix and body/function. Named class methods
are recognized in AS2/AS3. Syntax recognition does not perform capture analysis,
create tasks, evaluate bodies or emit executable closures.

## Implemented versus pending

| Part | State |
| --- | --- |
| Coco/R declaration/block/expression recognition | Implemented in AS1/AS2/AS3 |
| Ordinary identifier compatibility and malformed syntax refusal | Executed frontend tests |
| Numeric source-to-FIR profile | Explicit semantic refusal at the prefix; old artifact preserved |
| Runtime owner callbacks without frame/clock advancement | Existing `Async.post` / `fvm_async_post` |
| Actual frame-free drawing and detached pixels | Executed native regression below |
| `asinc` closure/task lowering and invocation | Not implemented |
| Cooperative scalar task input/result/cancellation owner | [Executable Limbo service](runtime/ASINC-TASKS.md); separate from AS lowering |
| AS lexical environment cells and task lowering | Pending |
| Concurrent physical threads sharing mutable AS objects | Not admitted |

The numeric executable profile emits **no synchronous fallback**. It reports
`asinc requires executable lexical-closure/task lowering, which is not available
in this profile`, with the original five-byte prefix span, and leaves an existing
output unchanged. These syntax examples cannot yet be compiled into executable
SWF/Dis by that profile. AS3 remains the documented AVM1 compatibility profile,
not an ABC implementation. No new SWF/Dis64 IDs or calling ABI are assigned.

## Execution contract to implement

A declaration marks invocation as frame-independent; defining the function does
not execute its body. A block posts work when reached. Invocation preserves
arguments, `this` and the lexical environment. Captured bindings have rooted, versioned per-task views, following the explicit
snapshot/publication contract below. This replaces the earlier shared-cell proposal. Returning from a posted body must not return
from the surrounding function. Task result/error/cancellation representation and
escape/type checking require dedicated FIR/optimizer/backend work.

Code touching managed objects builds/mutates the scene tree on the context owner
through a frame-independent task queue. Polling and eligible callbacks do not
wait for a SWF frame or clock tick. Immutable drawing commands go to independent
raster workers and publish damage when ready. Pure copied-data work may run in
physical workers; shared lexical cells and managed MovieClips cannot be handed
to those workers. This distinction is necessary to preserve scopes and GC while
keeping the timeline as a compatibility event source.

The existing owner-loop implementation queues eligible work for the next poll,
including with a frozen clock. It does not recurse inline or preempt a running
callback. A full `asinc` implementation must define task budgets/safepoints and
error/result ownership. Source recognition alone enables none of these new rules.
Cross-VM work continues through typed value/channel capabilities, never captured
native pointers into another heap. Shared Dis64 dependencies require agreement
in the Inferno/FlashVM journals before being enabled.

## Actual drawing proof

[Native regression](../disVM/runtime/test_frame_free_drawing.c) posts a genuine
drawing callback, collects the pending callback roots, polls at a fixed monotonic
instant and checks that the previously empty clip now has actual contours. No
SWF frame-advance call occurs. It captures independent path commands, destroys
the VM, rasterizes those commands and compares every RGBA byte with ordinary
MovieClip rendering. This is an existing runtime API proof, not execution of the
new `asinc` syntax or proof of physical parallel ActionScript.

After configuring/building the editor runtime:

```sh
cmake --build build/workspace/disvm-editor --target disvm_frame_free_drawing_test
build/workspace/disvm-editor/disvm_frame_free_drawing_test
```

[Commented source fixtures](examples/asinc/README.md) explain parser-only use and
explicit executable-profile refusal.

## Captured views and remote changes (agreed direction; implementation pending)

Each captured binding has a stable identity, not the identity of a boxed primitive.
Tasks retain rooted environments after the declaring function returns. Each task
and the frame owner see independent versions; unchanged storage is shared through
copy-on-write. Objects/arrays require nested-write tracking and alias-preserving
versions, not merely copied top-level pointers.

Before frame code begins, publish task-originated dirty bindings into the frame
view. After frame computation, publish frame-originated dirty bindings into task
views. Transfer only changes since the last publication in that direction.
Receiving a version does not create a new local write or echo it back. Publication
must preserve task writes made while frame computation runs. Concurrent writes
to the same binding/path require an explicit conflict policy; none is selected
here. Frame-independent execution and drawing continue between these boundaries;
an equivalent explicit boundary is still needed for hosts without frames.

The requested binding intrinsic is `variable.onChangeVarAsinc(callback)`.
It observes changes published by another asinc/task/thread, including primitive
bindings. The compiler resolves the binding before ordinary property lookup;
this is not a method attached to a number or a getter/setter on every object.
Callbacks are delivered on the subscriber owner queue after publication, outside
internal locks, independently of frame rendering. They do not execute on the
writer thread or recursively inside assignment. Own writes and unchanged echoed
versions do not trigger this remote-change notification.

Proposed event payload: binding identity, source stable task ID (and optional
thread ID), previous/new published values, publication version. Proposed inspection
operation returns a detached read-only map: `key: id_asinc/tid, value: var_val`.
The exact inspection method name and subscription cancellation handle remain to
be chosen. Stable task identity must distinguish reused OS thread IDs. Snapshots
expose coherent last-published values, never live cross-thread heap pointers or
unpublished partial writes. Whether every intermediate local assignment must be
retained as an event is not agreed: publication events alone do not promise that.
Lifetime, unsubscribe, task retirement and retained snapshot roots need explicit
runtime ownership. This API and capture synchronization are not executable yet.

[Executable scalar publication prototype / Исполняемый скалярный прототип](runtime/ASINC-VIEWS.md).
