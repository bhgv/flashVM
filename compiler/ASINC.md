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
| Lexical environment cells, task result/error/cancellation types | Design/implementation pending |
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
arguments, `this` and the lexical environment. Captured mutable variables need
shared rooted environment cells on the owning VM, not snapshots silently
substituted for lambda semantics. Returning from a posted body must not return
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
