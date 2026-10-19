# MovieClip global input methods — v0.627

[Русский](CLIP_INPUT_METHODS.ru.md). [`fvm_movieclip_dispatch_input`](../include/flashvm/clip_actions.h#L86) extends the
[retained scene route](CLIP_INPUT.md) with `onMouseMove`, `onMouseDown`, `onMouseUp`,
`onKeyDown` and `onKeyUp`. The SDL player now uses this combined route. Existing
[`fvm_movieclip_dispatch_clip_input`](../include/flashvm/clip_actions.h#L71) remains retained-bytecode-only with unchanged
work accounting. Selected [`fvm_movieclip_dispatch_clip_actions`](../include/flashvm/clip_actions.h#L55) is also unchanged.

## Host contract and source policy

Borrow/pin a same-context native MovieClip root and call on its owner thread.
Accept exactly one [`FVM_CLIP_EVENT_MOUSE_MOVE`](../include/flashvm/clip_actions.h#L8), [`FVM_CLIP_EVENT_MOUSE_DOWN`](../include/flashvm/clip_actions.h#L9),
[`FVM_CLIP_EVENT_MOUSE_UP`](../include/flashvm/clip_actions.h#L10), [`FVM_CLIP_EVENT_KEY_DOWN`](../include/flashvm/clip_actions.h#L11) or [`FVM_CLIP_EVENT_KEY_UP`](../include/flashvm/clip_actions.h#L12) bit.
The host publishes state separately; this API does not open a device or dispatch
Mouse/Key namespace listeners, select focus, tick, poll timers or present pixels.

| Source version returned for a recipient | Retained records | Named methods |
| --- | --- | --- |
| 1–4 | Traverse without delivery | None |
| 5 | Yes | None, including no getter/resolver lookup |
| 6–8 | Yes | Yes |
| 0, native/ambient unrestricted policy | Yes | Yes |

A native clip can inherit the context's ambient source policy; native creation
does not itself override that policy with version zero. The gate uses the retained
source version observed at that recipient's turn. Hidden/stopped clips and clips
without keyboard focus still receive these global events. Static artwork does not.
No targeted Press/Release/Roll/Drag, wheel method or automatic load/unload/construct
scheduler is added. A separate [v0.628 KeyPress route](CLIP_KEYPRESS.md) now follows
scene KeyDown; these five-event APIs still take no selector. This is an explicit bounded host policy,
not certification of exact original-player event order.

## Preparation, lookup and mutation

The same bounded, depth-sorted root-first batch is captured before any callback.
Newborns wait for the next call; removed or reparented pending branches skip.
At each receiver:

1. Revalidate captured ancestry/removal and obtain the retained source version.
2. Prepare and execute matching ClipActions using that receiver's current table.
3. For an eligible source, revalidate ancestry/removal again. A receiver removed
   by its own record does not receive its method in this call.
4. Charge one method bridge unit and resolve the corresponding zero-argument
   method once, with `this` equal to the receiver.

Function-valued own/inherited/getter results preserve the defining owner through
normal method dispatch. Lookup retains the context's ordinary case/resolver policy;
loading a MovieClip resource alone does not install a context SWF execution profile.
Mixed-profile original-player semantics remain outside this bounded contract.
Native registered methods use the existing native-entry
contract. Missing/noncallable members are ignored; actual getter, resolver and
callback errors propagate. There is no same-name global-function fallback.
Retained records can install/replace their own upcoming method. Earlier receivers
can change a later receiver's method. There is no entry-time function snapshot for
the complete tree. Reparenting/deletion performed by a method affects later turns.

A getter can itself mutate topology; lookup and its returned call form one ordinary
method-dispatch operation, without a second topology check inside that operation.
Native callbacks are synchronous and are not preempted by worker cancellation.

## Work, GC, errors and redraw

Capture/sort, retained records, repeated ancestry validation, method bridge and
AVM1 callbacks share one [`FVM_ExecutionWorkScope`](../src/core/execution_work_scope.hpp#L9). The bridge charges even if a
member is absent; full-route work therefore differs from the retained-only API.
The traversal bounds in `clip_input_constants.h` still apply. Cancellation remains
cooperative; one unit is not a wall-clock guarantee or a count of every property
lookup/allocation inside the shared dispatcher.

[GC:] The outer native barrier keeps the prepared graph and temporary handler
values alive. Explicit collection returns INVALID_ARGUMENT; incremental step does
not collect. Same-context recursive combined/retained tree entry and context
teardown are blocked until return. These context flags and the active batch are
not serialized. Function/scene graphs remain serializable through existing APIs.

Method lookup and execution grant `updateAfterEvent` eligibility, including getters
and nested calls, restoring the prior dynamic scope on every exit. The coalesced
pending redraw survives errors. The player presents after its existing callback
batch; methods do not secretly tick the timeline or render themselves.

Invalid root/owner/type/event/phase/reentry returns INVALID_ARGUMENT. Numeric work
exhaustion or cooperative stop returns ERROR; allocation failure returns
OUT_OF_MEMORY. Actual handler results, including UNSUPPORTED and EXCEPTION, retain
their meaning. Completed effects, input state and earlier callbacks stay committed;
errors skip the remaining batch. Calling again starts a new event, not a resume.

## SDL order and evidence

For each copied FIFO transition: publish native event-time state, dispatch namespace
callbacks, then combined scene delivery if successful and the reset epoch is unchanged.
With a scene root, keyboard publication uses [`fvm_input_dispatch_key_listeners`](../include/flashvm/keyboard.h#L97):
rooted Key observers only, without a separate implicit focused-clip call. The scene
then delivers each clip method once independently of focus. This prevents a duplicate
notification when the focused clip is also in the scene. Explicit Key.addListener
registration remains an independent notification source, even for a scene clip.
The new observer-only API shares the original keyboard dispatch preparation,
state-publication, error, reentry and GC contract; it does not query/change focus or
grant observer-only redraw eligibility. Hosts composing these APIs choose the same
order explicitly. The old [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90) and rootless SDL wrapper
retain their focus-first behavior. Primary touch uses the existing single-pointer mapping.
Recursive pumping remains invalid; callback appends wait for the next entry batch.
A failed method consumes its transition and leaves later queued transitions for a
future pump. A namespace failure/reset prevents the scene part of that transition.

- [Methods](../tests/clip_input_methods.cpp): all five events, SWF5–8 source gates,
  own/inherited/getter/native routes, old API isolation, missing/noncallable members.
- [Policy](../tests/clip_input_methods_policy.cpp): native version zero, hidden/stopped,
  resolver, teardown/reentry and invalid lifecycle/ownership calls.
- [Mutation](../tests/clip_input_methods_mutation.cpp): record-installed methods,
  self-removal and deferred newborns.
- [Errors](../tests/clip_input_methods_errors.cpp),
  [OOM](../tests/clip_input_methods_allocation.cpp),
  [work](../tests/clip_input_methods_budget.cpp),
  [workers](../tests/clip_input_methods_worker.cpp): committed failures, getter/native
  throws, allocation sweep, partial budget completion and actual OS-worker stop.
- [Graph/JIT](../tests/clip_input_methods_graph.cpp): inherited AVM1 methods after
  sender destruction and traced-string collection, interpreter/actual JIT.
- [Keyboard composition](../tests/keyboard_scene_listeners.cpp) and
  [allocation](../tests/keyboard_scene_allocation.cpp): observer-only publication,
  exactly one focused scene call, unchanged legacy focus route, state/error/reentry
  and allocation cleanup.
- [SDL](../frontend/tests/clip_input.c): namespace → record → method order, event-time
  coordinates/held keys, primary touch, focused actor without duplicates, method/record errors and deferred appends.
- [Player](../frontend/tests/clip_input_methods.py) with the existing
  [queue injector](../frontend/tests/clip_player.c): independent SWF5–8 method-only
  and record/method decimal traces, wrong-value controls, native-entry stats and
  method-originated coalesced redraw. Uses actual SDL and production host logic.

The [C89 tutorial](../examples/embedding/clip_actions/README.md) demonstrates all
three APIs without SDL. Linux SDL dummy tests are synthetic input evidence, not
physical-device, visible-window or Windows/macOS/Android acceptance. All nine
roadmap stages remain open.
v0.627 validation: 275 selected Release CTests, 93 ASan/UBSan CTests and a strict
C89 sanitizer run passed. A separately linked player using the v0.626 retained-only
SDL adapter rejected the same SWF8 record/method trace movie. Leak detection was
disabled; see [stage status](ROADMAP_STATUS.md) for the limits of this evidence.

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
| `FVM_ExecutionWorkScope` | class | [src/core/execution_work_scope.hpp:9](../src/core/execution_work_scope.hpp#L9) |
| `fvm_input_dispatch_key_listeners` | function | [include/flashvm/keyboard.h:97](../include/flashvm/keyboard.h#L97) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:55](../include/flashvm/clip_actions.h#L55) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:71](../include/flashvm/clip_actions.h#L71) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:86](../include/flashvm/clip_actions.h#L86) |
<!-- source-links:end -->
