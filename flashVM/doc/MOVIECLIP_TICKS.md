# MovieClip ticks and optional handlers — v0.624

[Русский](MOVIECLIP_TICKS.ru.md). [fvm_movieclip_advance](../include/flashvm/movieclip.h#L66) drives synchronous owner-thread
MovieClip ticks. It advances eligible timelines, runs retained initializers and frame
scripts, delivers onEnterFrame, then visits a depth-ordered child snapshot. Stopped
and single-frame clips still receive the handler. No timer poll, rendering or device
presentation occurs; see [redraw](UPDATE_AFTER_EVENT.md) for independently hosted draws.

v0.629 correction: the earlier implementation copied an insertion-ordered hash
table, despite describing it as depth-ordered. Child snapshots now validate identities,
use budgeted ascending depth sorting and skip effective removed ancestors.
[Exact ordering, bounds and changed work](TIMELINE_DEPTH_ORDER.md). The childless
exact two-unit policy remains; child-bearing ticks now also charge sorting/query work.

## Shared work and partial completion

The complete call now enters [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9) once. Multiple ticks, descendants,
nested advance calls and AVM1 callbacks share the current outer instruction budget.
Only an outer host call resets the counter. [FVM_Config](../include/flashvm/flashvm.h#L51).instruction_limit equal to zero
disables the numeric bound, while cooperative worker cancellation remains enabled.
Exhaustion or a polled worker stop returns [FVM_ERROR](../include/flashvm/flashvm.h#L43); it is not a script exception.

Charged native work consists of one node entry (including static art), one optional
handler delivery, each entry staged into an initializer/action-segment/child snapshot,
and each initializer or segmented action-block delivery. AVM1 instructions retain their
existing accounting. Child snapshot entries are charged before its allocation. A
single-frame clip with no scripts or children consumes two units per tick even when
its handler is absent. Zero ticks does no work and invokes no callback. The maximum
requested ticks and descendant depth are [FVM_MOVIECLIP_MAX_TICKS_PER_CALL](../src/standard/movieclip_constants.h#L33) and
[FVM_MOVIECLIP_ADVANCE_MAX_DEPTH](../src/standard/movieclip_constants.h#L27); exceeding them returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45).

This is an accounting/cancellation boundary, not hard real-time preemption. Scene
placement/resource helpers, native callback internals, property lookup, allocation,
rendering and other uncharged work are not made fully bounded by these units. Native
callbacks must cooperate themselves. Worker stops are observed on the first charged
unit and then every 64 units using the existing execution-work polling policy.

Failures do not roll back completed effects. A playhead may already have changed or
an action/initializer/handler may have executed before later [FVM_ERROR](../include/flashvm/flashvm.h#L43),
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) or [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48). Remaining handlers/children/ticks are skipped.
The next call continues current committed state; it does not resume a hidden cursor
through the failed batch. Do not blindly replay a batch after an error.

## Optional method lookup and scene mutation

onEnterFrame now uses [fvm_object_call_optional](../src/capi/value_c.cpp#L1368), like other explicit event routes.
Own/inherited Function properties, getters and native method registrations execute
with the original clip receiver. Defining-owner and superclass metadata survive
lookup; a getter executes once. Missing or noncallable members are ignored; errors
from a real getter or handler propagate. A missing handler never calls a same-named
global. This uses ordinary method lookup, including its existing SWF case and
resolver rules, rather than the extension metatable read fallback. Both native and
script Functions were callable before this revision; this is not newly added AS
Function support or a general fix to all original-player superclass semantics.

The current node's scene/actions/handler execute before its child snapshot is taken.
Children created by that parent handler can be visited in the same tick. Changes
made by a later child handler cannot append to the parent's already captured batch.
A snapshot child whose physical parent is no longer this node is skipped. Ordering
is the native child-depth order, not an assertion of every original Player ordering.
Visibility does not suppress timeline delivery. Full ClipActions/load/unload and
automatic input hit-target fanout remain outside this work.

## Ownership and errors

The public API borrows a same-context clip; pin or otherwise retain it across host
calls. Use only the context owner thread. Null/foreign clips, destroying/collecting/
releasing phases, existing sound-callback reentry and excessive tick count return
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before work is entered. Zero ticks still validates these inputs.
A depth error is discovered during traversal and can follow committed earlier work.

[GC:] The enclosing [FVM_NativeCallGuard](../src/core/native_call_guard.hpp#L9) remains active throughout advance, including
callbacks. Inside that barrier [fvm_gc_collect](../include/flashvm/gc.h#L44) returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); [fvm_gc_step](../include/flashvm/gc.h#L90)
returns [FVM_OK](../include/flashvm/flashvm.h#L42) with no collection. Collect explicitly at a later safe host boundary.
This revision does not permit collecting raw scene snapshots during callback
execution. Root/owner/work scopes unwind on normal results, errors and OOM. Host
callback userdata is borrowed and must outlive its registration/context. Function
and clip graph transfer retain supported managed state; no active work scope or
native stack snapshot is serialized.

## Evidence and example

- [dispatch](../tests/movieclip_tick_dispatch.cpp): own/inherited/getter owner identity,
  native methods, ignored members/global, error/exception commitment, lifecycle phases.
- [budget](../tests/movieclip_tick_budget.cpp): exact tick units, zero ticks, child
  staging and nested host calls sharing the limit.
- [frame actions](../tests/movieclip_tick_actions.cpp): distinct frame scripts cannot
  reset a batch budget in interpreter or actual native JIT.
- [mutation](../tests/movieclip_tick_mutation.cpp): parent-created child, removed
  sibling and deferred newborn from a child handler.
- [workers](../tests/movieclip_tick_worker.cpp): actual OS workers stopped before
  entry and during snapshot staging, with numeric limit disabled.
- [OOM](../tests/movieclip_tick_allocation.cpp): allocation sweep, restored scopes,
  usable committed state and later collection.
- [graphs](../tests/movieclip_tick_graph.cpp): imported inherited AVM1 handler after
  sender destruction and traced-string collection.
- [JIT](../tests/jit_movieclip_tick.cpp): SWF6–8 inherited superclass calls in
  interpreter/manual-native/automatic-native execution with native-entry checks.

A temporary executable linked against the v0.623 movieclip implementation rejects
the new owner-metadata and shared-budget controls. The superclass-call fixture also
passes on that old implementation and is regression evidence only. None of these
runs certifies original-player conformance or Windows/macOS/Android device behavior.

The [C89 tutorial](../examples/embedding/movieclip/README.md) stops a one-frame clip,
requests three ticks with a five-unit budget, retains two callbacks after the limit
error and obtains a fresh budget on its next outer call. The companion ActionScript
file remains a separate sketch for an external compiler. All roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:66](../include/flashvm/movieclip.h#L66) |
| `FVM_MOVIECLIP_ADVANCE_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:27](../src/standard/movieclip_constants.h#L27) |
| `FVM_MOVIECLIP_MAX_TICKS_PER_CALL` | macro / define | [src/standard/movieclip_constants.h:33](../src/standard/movieclip_constants.h#L33) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:9](../src/core/native_call_guard.hpp#L9) |
| `fvm_object_call_optional` | function | [src/capi/value_c.cpp:1368](../src/capi/value_c.cpp#L1368) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
