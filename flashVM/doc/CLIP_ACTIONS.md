# Retained SWF ClipActions — v0.625

[Русский](CLIP_ACTIONS.ru.md). PlaceObject2 can now retain placement-specific
ClipActions on sprite instances instead of rejecting that entire optional field.
The loader copies and inspects action blocks without executing them. A normal
[fvm_movieclip_advance](../include/flashvm/movieclip.h#L62) delivers matching EnterFrame records, after the clip's frame
actions and before its optional onEnterFrame method. Other events have an explicit
owner-thread C89 entry point; they are not automatically selected by the player.

## File format and strict profile

The [Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf),
ClipActions/ClipEventFlags sections, defines the optional placement records and their
version-dependent fields. This implementation uses these wire values and a strict
file-version policy; it does not reproduce every lenient original-player recovery.

| Source SWF | Event mask profile | Flag width |
| --- | --- | --- |
| 5 | Load, EnterFrame, Unload, MouseMove/Down/Up, KeyDown/Up, Data | UI16 |
| 6 | Previous events plus Initialize, Press, Release, ReleaseOutside, RollOver/Out, DragOver/Out, KeyPress | UI32 |
| 7–8 | Previous events plus Construct | UI32 |

The reserved prefix must be zero, reserved upper flag bits are invalid, and
AllEventFlags must equal the union of the record masks. A complete zero end marker
is required, with no trailing bytes. An empty table is accepted. Truncated sizes,
selectors, action records and markers are rejected within their placement boundary.
KeyPress contributes one raw key byte to ActionRecordSize; that byte is excluded
from the action block passed to AVM1. The existing block inspector accepts an
implicit ActionEnd at the exact bounded end. Unsupported AVM1 remains unsupported.

One placement accepts at most [FVM_SWF_CLIP_ACTION_MAX_RECORDS](../src/swf/clip_action_constants.h#L15) records,
[FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES](../src/standard/movieclip_constants.h#L21) action bytes per record and
[FVM_SWF_CLIP_ACTION_MAX_BYTES](../src/swf/clip_action_constants.h#L16) action bytes in total. These are implementation bounds.
Oversized or malformed records return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); events outside the strict
source-version profile and ClipActions on static artwork return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).
The separate clipDepth field remains unsupported. This does not add PlaceObject3,
DefineButton actions, event hit-target selection or a textual AS compiler.

## C89 selected-event delivery

Include `flashvm/clip_actions.h`. Call [fvm_movieclip_dispatch_clip_actions](../include/flashvm/clip_actions.h#L37) with:

- The owning context and its retained MovieClip instance.
- Exactly one FVM_CLIP_EVENT_* bit from the public header, not an event combination.
- A raw SWF KeyPress selector from 0 through [FVM_SWF_CLIP_ACTION_MAX_KEY](../src/swf/clip_action_constants.h#L18) for
  [FVM_CLIP_EVENT_KEY_PRESS](../include/flashvm/clip_actions.h#L22); zero for every other event.

The call runs matching bytecode records in source order with the selected clip as
receiver. A combined record runs once for the selected event, with key filtering
only when that selected event is KeyPress. It does not call an onX method, change
Key/Mouse/focus state, choose a hit target, detach a clip, advance timeline, poll a
timer or render. Missing/empty tables and events without matches succeed. The API
has no returned delivery count. A native empty clip uses the host profile; retained
sprite libraries retain their source SWF policy even after graph transfer.

The host controls event selection and once-only lifecycle policy. Calling Load,
Construct, Initialize or Unload twice delivers twice; no hidden loader cursor is
created. Explicit EnterFrame followed by advance also delivers twice. Load/unload, construct/initialize and data remain explicit. Since v0.626 the desktop
also broadcasts the five global MouseMove/Down/Up and KeyDown/Up records through
[the selected scene](CLIP_INPUT.md); EnterFrame remains delivered by ticks. Targeted
interaction/lifecycle scheduling remains open. Native Mouse/Key/focus APIs retain
their separate contracts.

All calls are synchronous on the context owner thread. The API borrows the clip;
pin it or retain its enclosing scene across host calls. Null, foreign or nonclip
arguments, static art, lifecycle/sound-callback reentry phases, invalid event bits
and invalid key arguments return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before delivery. A valid
event unavailable in the clip's strict SWF profile returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).

## Snapshots, work and errors

Preparation charges each captured record and action byte, checks contiguous record
indices, selectors, masks, code bounds and lowercase hex, then snapshots scalar
selectors and code values. The selected AVM1 block is checked/executed by the bounded
block executor. Allocation failure or invalid preparation metadata executes no
handler. A callback can replace the live table or mutate later record metadata;
this batch still uses its captured selectors/code. Future deliveries see the change.

Delivery, preparation and AVM1 share [FVM_ExecutionWorkScope](../src/core/execution_work_scope.hpp#L9) with an enclosing
advance or script call. A public outer dispatch also charges one entry unit; matching
records charge a delivery unit. Native work polls worker cancellation on its normal
first/every-64-unit policy. The numeric limit can be disabled independently of stop.
Native callback internals and other helpers are not made hard real-time preemptible.

Budget/stop, [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44), real handler errors and [FVM_EXCEPTION](../include/flashvm/flashvm.h#L48) skip the rest
of the batch and preserve completed effects. A later selected bytecode error may
follow earlier delivered records. There is no transaction rollback or resumable
batch cursor. Clear/handle script exceptions with the existing exception API before
continuing, and do not blindly replay an errored event batch.

MouseMove/Down/Up and KeyDown/Up records grant dynamic updateAfterEvent eligibility;
EnterFrame and other routes do not grant it. An eligible enclosing interval/input
chain remains eligible. Scope restoration occurs on every error/exception/OOM path;
a committed redraw request remains pending. [Redraw contracts](UPDATE_AFTER_EVENT.md).

[GC:] The full native barrier remains active. [fvm_gc_collect](../include/flashvm/gc.h#L44) from a callback returns
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); [fvm_gc_step](../include/flashvm/gc.h#L90) returns [FVM_OK](../include/flashvm/flashvm.h#L42) without collection. Collect at a
later host safe point. Captured code remains valid even if its managed owner edge
is replaced during delivery. No native userdata or active execution scope is stored
in the handler graph. Retained code is owned ASCII hex, not a borrowed file pointer.

## Scene changes, duplicate and transfer

Placement without the ClipActions field preserves an existing table. A present
field replaces the receiving instance's edge, including an explicitly empty table.
Seek/loop composition carries the most recent authored field through the existing
scene reconstruction policy. This is separate from each instance's timeline frames.
Dynamic duplicates retain the immutable placement handler table while owning their
own future replacement edge. No ordinary dynamic onEnterFrame property-copying
semantics are added by that retention.

Generic graph serialization carries the table, selectors, strings, library and
source version. After sender destruction, a rooted imported sprite can execute its
retained records. Existing per-instance state and native hooks retain their own
transport rules. This is not a new shared cross-thread display object.

## Executable evidence

- [Wire/events](../tests/swf_clip_actions.cpp), [malformed](../tests/swf_clip_actions_malformed.cpp):
  independent raw SWF5–8 flags, selectors, prefix truncations and strict version gates.
- [Scene/duplicate](../tests/swf_clip_actions_update.cpp): modification, stopped ticks,
  repeated explicit delivery and duplicated handler retention.
- [Graphs](../tests/swf_clip_actions_graph.cpp): sender teardown, traced-string GC,
  source-version retention and subsequent explicit/tick execution.
- [Budget/errors](../tests/swf_clip_actions_budget.cpp), [workers](../tests/swf_clip_actions_worker.cpp):
  shared limits, committed effects, exceptions and actual OS-thread cancellation.
- [Snapshot](../tests/swf_clip_actions_snapshot.cpp), [corrupt graphs](../tests/swf_clip_actions_corrupt_graph.cpp):
  callback replacement/metadata mutation and rejected malformed preparation.
- [OOM](../tests/swf_clip_actions_allocation.cpp): parser/graph-building, first-placement/tick and event-dispatch
  allocation sweeps, restored scopes and usable later calls.
- [Redraw](../tests/swf_clip_actions_redraw.cpp), [JIT](../tests/jit_clip_actions.cpp):
  event eligibility, lifecycle gates and interpreter/automatic-native equivalence.
- [Actual player](../frontend/tests/clip_actions.py): SDL dummy-video SWF5–8 runs in
  interpreter/JIT plus wrong-value controls. Dummy video is not a visible device test.

The [public API checks](../tests/clip_actions_api.cpp) cover native empty clips,
owner/type validation, sound/lifecycle restrictions, argument masks and late SWF4
loading. A temporary executable linked to the v0.624 timeline loader rejects the
new positive ClipActions wire fixture.

[C89 example](../examples/embedding/clip_actions/README.md) embeds a small SWF and
explicitly selects Load after the first automatic EnterFrame. Complete ClipActions
scheduling/event ordering, automatic input targets, codecs and foreign-device
acceptance remain open. All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_EVENT_KEY_PRESS` | macro / define | [include/flashvm/clip_actions.h:22](../include/flashvm/clip_actions.h#L22) |
| `FVM_EXCEPTION` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `fvm_gc_collect` | function | [include/flashvm/gc.h:44](../include/flashvm/gc.h#L44) |
| `fvm_gc_step` | function | [include/flashvm/gc.h:90](../include/flashvm/gc.h#L90) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:62](../include/flashvm/movieclip.h#L62) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:37](../include/flashvm/clip_actions.h#L37) |
| `FVM_MOVIECLIP_MAX_FRAME_ACTION_BYTES` | macro / define | [src/standard/movieclip_constants.h:21](../src/standard/movieclip_constants.h#L21) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_SWF_CLIP_ACTION_MAX_BYTES` | macro / define | [src/swf/clip_action_constants.h:16](../src/swf/clip_action_constants.h#L16) |
| `FVM_SWF_CLIP_ACTION_MAX_KEY` | macro / define | [src/swf/clip_action_constants.h:18](../src/swf/clip_action_constants.h#L18) |
| `FVM_SWF_CLIP_ACTION_MAX_RECORDS` | macro / define | [src/swf/clip_action_constants.h:15](../src/swf/clip_action_constants.h#L15) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
