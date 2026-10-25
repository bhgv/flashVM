# Retained input across a MovieClip scene — v0.626

[Русский](CLIP_INPUT.ru.md). [fvm_movieclip_dispatch_clip_input](../include/flashvm/clip_actions.h#L76) broadcasts one of
five global retained ClipActions through a selected native MovieClip subtree:
[FVM_CLIP_EVENT_MOUSE_MOVE](../include/flashvm/clip_actions.h#L8), [FVM_CLIP_EVENT_MOUSE_DOWN](../include/flashvm/clip_actions.h#L9), [FVM_CLIP_EVENT_MOUSE_UP](../include/flashvm/clip_actions.h#L10),
[FVM_CLIP_EVENT_KEY_DOWN](../include/flashvm/clip_actions.h#L11) or [FVM_CLIP_EVENT_KEY_UP](../include/flashvm/clip_actions.h#L12). Exactly one bit is required.
It complements [selected-record delivery](CLIP_ACTIONS.md); it does not replace
native Mouse/Key listeners, focus notifications or a full Flash event scheduler.

## C89 contract

The API borrows the owning context and retained root on the context owner thread.
Pin the root or otherwise keep its graph reachable between host calls. The host
publishes pointer/keyboard state first, for example with [fvm_input_dispatch_mouse](../include/flashvm/mouse.h#L37)
or [fvm_input_dispatch_keyboard](../include/flashvm/keyboard.h#L108). This API itself does not change native input state,
advance timeline, poll timers, render, choose focus or invoke any native onX method.
The caller controls state publication and the namespace/tree order.

Visibility, playback stop and focus do not filter these five global records. Static
artwork is inspected but has no retained handler. Source nodes older than SWF5 are
traversed without delivery; their supported descendants remain eligible. Native
clips without a source library use the existing ambient/host version policy. Input
coordinates and Key polling therefore reflect host state at the actual callback.
A callback's explicit later state changes are not rolled back or frozen by this API.

Null, foreign, nonclip or static-art roots, lifecycle/sound-callback phases, unsupported
or combined event bits and recursive tree delivery return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47).
The root/type checks and topology preparation are inside the allocation exception
boundary; their OOM returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L46). Missing handlers are successful.
Normal record execution can return its own error or script exception. No output
count, persistent batch cursor or implicit retry is created.

## Prepared topology and later handler lookup

Before the first callback, capture the complete native subtree in root-first order,
with each parent's children sorted by native depth. Sorting uses a charged heap
rather than insertion order or a quadratic insertion sort. Preparation validates
native ownership/type, rejects repeated live nodes/cycles and can normalize legacy
child-slot tables. Such internal normalization is not a script callback or rollback
transaction. A failed capture invokes no user handler.

A recipient stores its clip identity and captured parent index. Before delivery,
validate its current captured ancestry and effective removed state. Removing or
moving a captured branch suppresses its still-pending descendants, including when
an earlier ancestor already ran. New children created by callbacks wait for the
next tree broadcast. Changing depth does not reorder this already prepared batch.
An explicitly selected root remains the root identity of this call; this is not a
new global display registry.

At each recipient's turn, [fvm_movieclip_dispatch_clip_actions](../include/flashvm/clip_actions.h#L58) prepares that receiver's
selector/code snapshot. Earlier receivers may replace a later receiver's table;
that later turn observes the replacement. Within the selected-record call, changes
to later records cannot alter its already captured selectors/code. These are two
different snapshot boundaries, not a freeze of every handler at tree-entry time.

## Work, errors and GC

One [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L11) covers node capture, child-slot staging, heap sorting,
each recipient turn, captured-ancestry comparisons, selected-record entry, action
bytes, matching record delivery and AVM1 instructions. Nested calls share the
outer budget; only an outer host call starts a fresh counter. An empty live native
root with no children/handlers uses three units. The numeric limit can be disabled
without disabling cooperative worker cancellation.

| Bound | Meaning |
| --- | --- |
| [FVM_CLIP_INPUT_MAX_DEPTH](../src/standard/clip_input_constants.h#L8) | Selected-root descendant depth, root at zero. |
| [FVM_CLIP_INPUT_MAX_NODES](../src/standard/clip_input_constants.h#L9) | Total inspected native nodes, including static artwork. |
| [FVM_CLIP_INPUT_MAX_CHILDREN](../src/standard/clip_input_constants.h#L10) | Child-depth sorting entries for one parent. |
| [FVM_CLIP_INPUT_MAX_RECIPIENTS](../src/standard/clip_input_constants.h#L11) | Prepared live receiver count. |

Depth/count violations reject preparation with [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47). Budget exhaustion
or a polled worker stop returns [FVM_ERROR](../include/flashvm/flashvm.h#L45). Completed callbacks, native state and
redraw requests remain committed after later budget/OOM/error/exception failure;
remaining receivers are skipped. A subsequent call is a new event delivery. Native
helpers, hash operations, allocation and callback internals are not hard real-time
preemptible. Cancellation follows the existing charged-work polling policy.

[GC:] [FVM_NativeCallGuard](../src/core/native_call_guard.hpp#L11) covers raw topology, receiver and code snapshots throughout
the call. [fvm_gc_collect](../include/flashvm/gc.h#L54) from a callback returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47);
[fvm_gc_step](../include/flashvm/gc.h#L116) returns [FVM_OK](../include/flashvm/flashvm.h#L44) without collecting. Collect at a later host safe point.
The clip_input_dispatching flag rejects tree reentry and context teardown; RAII
restores it on every return and allocation exception. Neither flag nor active batch
is graph-serialized. Managed scene/code graphs retain their existing transport rules.

All five events grant the existing dynamic updateAfterEvent eligibility during their
retained bytecode. Earlier committed requests survive errors. A supported outer
interval/input scope stays eligible. This does not itself display any pixels.

## SDL player integration

The shipped player supplies its pinned root to
fvm_desktop_window_pump_clip_input. For each existing FIFO transition it:

1. Restores authored world coordinates and publishes the copied input snapshot.
2. Delivers Mouse/Key namespace handlers; since v0.627 the scene-root keyboard route
   omits the separate focused call, avoiding duplicate clip methods. Rootless hosts
   keep the original focus-first keyboard delivery.
3. If that stage succeeded and the reset epoch still matches, broadcasts the retained
   scene records and, since v0.627, eligible named methods through
   [`fvm_movieclip_dispatch_input`](../include/flashvm/clip_actions.h#L93) before the next queued transition
   ([method contract](CLIP_INPUT_METHODS.md)).

The legacy fvm_desktop_window_pump_input and keyboard wrapper remain namespace-only.
The new entry point borrows the root for the synchronous main-thread call. A namespace
error or reset prevents the tree stage of that transition. A tree error leaves prior
state/callbacks committed, consumes this transition and leaves later queued transitions
for a future pump. Recursive window pumping remains invalid; callbacks appended to
SDL/FIFO wait for the next entry batch. Focus/background loss still cancels old batches.

Primary touch uses the existing single-contact down/move/up mapping; secondary contacts
and generated mouse duplicates retain their existing filtering. No keyboard/mouse APK
or new Android input assumption is introduced. A separate
[v0.628 KeyPress route](CLIP_KEYPRESS.md) now follows scene KeyDown.
Wheel ClipActions,
Press/Release/Roll/Drag hit targets and automatic lifecycle scheduling remain open.
The retained-only API itself still invokes no named methods. No complete original-player ordering is certified.

## Evidence and example

- [Core](../tests/clip_input.cpp), [order](../tests/clip_input_order.cpp),
  [topology](../tests/clip_input_topology.cpp), [mutation](../tests/clip_input_mutation.cpp):
  five events, invisible/stopped clips, insertion-independent depths, malformed/deep
  graphs, removed branches and deferred newborns.
- [Budget](../tests/clip_input_budget.cpp), [OOM](../tests/clip_input_allocation.cpp),
  [workers](../tests/clip_input_worker.cpp), [state/guards](../tests/clip_input_state.cpp):
  shared limits, committed effects, allocation sweep, actual OS-worker stops,
  native state, blocked collection/teardown/reentry, exceptions and lifecycle phases.
- [Graph/JIT](../tests/clip_input_graph_jit.cpp): sender destruction, traced strings,
  imported scene broadcast and actual automatic-native entries.
- [SDL adapter](../frontend/tests/clip_input.c): real SDL dummy video with synthetic
  mouse/finger/key messages, event-time coordinates/held keys, namespace-before-record
  order, callback appends, partial-error recovery and recursive-pump guards.
- [Actual player](../frontend/tests/clip_input_player.py) with
  [SDL queue injector](../frontend/tests/clip_player.c): SWF5–8 in interpreter/JIT,
  five events after the first presentation, summed script marker and one coalesced
  interframe redraw. Wrong-value controls must fail. The production adapter is used;
  injection is confined to the separate test executable.

[C89 tutorial](../examples/embedding/clip_actions/README.md) separately publishes
pointer state, broadcasts MouseMove and consumes a redraw flag without a renderer.
These Linux dummy-video/protocol results are not physical input, visible-device or
Windows/macOS/Android acceptance. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_EVENT_KEY_DOWN` | macro / define | [include/flashvm/clip_actions.h:11](../include/flashvm/clip_actions.h#L11) |
| `FVM_CLIP_EVENT_KEY_UP` | macro / define | [include/flashvm/clip_actions.h:12](../include/flashvm/clip_actions.h#L12) |
| `FVM_CLIP_EVENT_MOUSE_DOWN` | macro / define | [include/flashvm/clip_actions.h:9](../include/flashvm/clip_actions.h#L9) |
| `FVM_CLIP_EVENT_MOUSE_MOVE` | macro / define | [include/flashvm/clip_actions.h:8](../include/flashvm/clip_actions.h#L8) |
| `FVM_CLIP_EVENT_MOUSE_UP` | macro / define | [include/flashvm/clip_actions.h:10](../include/flashvm/clip_actions.h#L10) |
| `FVM_CLIP_INPUT_MAX_CHILDREN` | macro / define | [src/standard/clip_input_constants.h:10](../src/standard/clip_input_constants.h#L10) |
| `FVM_CLIP_INPUT_MAX_DEPTH` | macro / define | [src/standard/clip_input_constants.h:8](../src/standard/clip_input_constants.h#L8) |
| `FVM_CLIP_INPUT_MAX_NODES` | macro / define | [src/standard/clip_input_constants.h:9](../src/standard/clip_input_constants.h#L9) |
| `FVM_CLIP_INPUT_MAX_RECIPIENTS` | macro / define | [src/standard/clip_input_constants.h:11](../src/standard/clip_input_constants.h#L11) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:11](../src/core/execution_work_scope.hpp#L11) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:54](../include/flashvm/gc.h#L54) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:116](../include/flashvm/gc.h#L116) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:108](../include/flashvm/keyboard.h#L108) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:37](../include/flashvm/mouse.h#L37) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:58](../include/flashvm/clip_actions.h#L58) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:76](../include/flashvm/clip_actions.h#L76) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:93](../include/flashvm/clip_actions.h#L93) |
| `FVM_NativeCallGuard` | class | [src/core/native_call_guard.hpp:11](../src/core/native_call_guard.hpp#L11) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
