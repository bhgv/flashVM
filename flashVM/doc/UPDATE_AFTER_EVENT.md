# updateAfterEvent and coalesced redraw — v0.623

[Русский](UPDATE_AFTER_EVENT.ru.md). [fvm_standard_install](../include/flashvm/standard.h#L20) now publishes
`updateAfterEvent()` for host/SWF5–8, with stable native graph ID
[FVM_STAGE_UPDATE_CALLBACK](../src/standard/stage_update_constants.h#L5). It returns undefined and ignores extra arguments
without coercion. Calls below SWF5 return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46), including installation
before loading an older SWF. Installation preserves conflicting own bindings
under the active SWF case rules.

## Eligible routes and lifetime

The global records a request only in a dynamically eligible call chain:

- A repeating setInterval Function or object/method callback.
- Native Mouse move/down/up listener delivery.
- The selected native MovieClip's onKeyDown/onKeyUp delivery.

Ordinary direct calls, standalone setTimeout callbacks, Mouse wheel, Key observers,
Selection, AsBroadcaster and onEnterFrame do not grant eligibility themselves.
A nested function or otherwise ineligible route retains an already eligible
outer interval/input scope. A MovieClip that is both focus and a Key observer
is eligible only for its focused-recipient invocation, not its later observer
invocation. Full automatic ClipActions/hit-target fanout is not added here.

[FVM_StageUpdateEventScope](../src/standard/stage_update.hpp#L9) restores the enclosing eligibility on successful return,
errors, script exceptions and OOM. A request already made remains pending after a
later callback failure. [GC:] These context booleans retain no managed pointers;
GC during a script/native-body callback does not consume the request or end the
scope. Existing native-call barriers remain active. The global itself allocates
nothing, performs no user coercion, and never renders recursively. Ordinary
Function/global call machinery can still allocate.

Repeated requests coalesce into one context-local flag. Function graph transfer
preserves callback identity but exports neither pending redraw state nor event
eligibility. An imported Function obeys the destination's current call chain.
The setting is not a serialized movie property or cross-context display handle.

## C89 host API

All three operations belong on the context owner thread, allocate nothing and
invoke no callbacks. They reject null context and destruction/collection/release
phases with [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45), preserving pending state and supplied outputs.
They do not require standard installation or an ActionScript event scope.

| API | Contract |
| --- | --- |
| [fvm_stage_request_update](../include/flashvm/stage.h#L38) | Set the coalesced flag, including an explicit host request outside script events. No timeline/render/device work is performed. |
| [fvm_stage_get_update](../include/flashvm/stage.h#L41) | Read the pending flag as 0/1 into a required `int *`; no consumption or ownership transfer. |
| [fvm_stage_take_update](../include/flashvm/stage.h#L45) | Read and clear into a required `int *`. Consume before drawing, so a later request remains pending. If a failed presentation is retried, request again. |

A renderer owns presentation and its error policy. Reading/consuming the flag
does not by itself prove that pixels reached a window. Standalone VM hosts may
use any display adapter and may inspect the flag after a recoverable script error.
Host requests are independent of the global's SWF version/event restrictions.

## Common desktop player

The active loop consumes a request before rendering the current scene between
frames. [clear_stage](../frontend/common/player.c#L73) and [fvm_movieclip_render_view](../include/flashvm/movieclip.h#L67) feed the same presenter as normal
frames; no timeline step, frame-script rerun or authored deadline increment occurs.
A request coinciding with an ordinary frame is satisfied by that frame. A request
made during presentation stays pending and prevents sleeping past another draw.
Background suspension and the bounded final-frame hold retain their existing
policy. Render failure terminates this player and releases its context.

`--stats` adds `presentations=N refreshes=M`, keeping the existing `frames=...`
line: refreshes count successful interframe redraws, and presentations count all
successful host presentations. Timeline ticks remain separately counted. This is
not an exact Flash Player timing/event-order certificate or latency benchmark.

## Executable evidence

- [Eligibility/host API](../tests/stage_update.cpp): host/SWF4–8, intervals and methods,
  timeout/Mouse/wheel/focus/Key distinctions, coalescing, errors and lifecycle guards.
- [Scopes/GC](../tests/stage_update_scopes.cpp): duplicate focus/observer delivery,
  inherited scope in a nested wheel call, explicit collecting script bodies and
  restoration after errors/exceptions.
- [OOM](../tests/stage_update_allocation.cpp): allocation-free host operations and
  eligibility restoration at every failed interval activation allocation.
- [Versions/conflicts](../tests/stage_update_versions.cpp): folded/exact property and
  native-registration conflicts, late SWF4 load, ignored coercible objects and phases.
- [Graphs](../tests/stage_update_graph.cpp): export in an eligible sender callback,
  sender teardown, no exported flags, rooted imported Function invocation after GC.
- [Native JIT](../tests/jit_stage_update.cpp): direct/alias calls, interval and Mouse
  delivery in interpreter/manual native/automatic JIT modes with native-entry checks.
- [Player/pixels](../frontend/tests/update_after_event.py): separate real SDL
  dummy-video runs and [protocol capture backend](../frontend/tests/update_capture.c).

The protocol backend uses a deterministic clock and the actual VM/player/rasterizer,
without a native window. At 2 fps, an interval every 10 ms hides a red scene, calls
the global twice per callback, and the second frame restores red. There must be
exactly 49 white interframe presentations at 10–490 ms between red frames at 0/500 ms.
The no-request control must contain only two red frames. Thus requests coalesce
and redraws do not advance timeline. A temporary player using the v0.622 loop
missed every white interframe pixel. Real SDL dummy-video runs verify successful
presenter calls/counts separately; neither fixture claims visible-device acceptance.

[C89 tutorial](../examples/embedding/timers/README.md) observes/consumes an interval's
request and checks explicit-host coalescing without needing a graphics backend.

Reference: [Adobe AS2 global-functions reference](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html),
updateAfterEvent entry, gives Flash 5 availability and restricted event/interval
use with rendering independent of the movie FPS. Exact original-player behavior,
complete MovieClip/ClipActions routes and foreign-platform/device acceptance remain open.
No whole roadmap stage is closed by this bounded implementation.

Since v0.625 selected retained MouseMove/Down/Up and KeyDown/Up ClipActions also
grant eligibility; other retained events do not grant it. [ClipActions](CLIP_ACTIONS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:67](../include/flashvm/movieclip.h#L67) |
| `fvm_stage_get_update` | function | [include/flashvm/stage.h:41](../include/flashvm/stage.h#L41) |
| `fvm_stage_request_update` | function | [include/flashvm/stage.h:38](../include/flashvm/stage.h#L38) |
| `fvm_stage_take_update` | function | [include/flashvm/stage.h:45](../include/flashvm/stage.h#L45) |
| `FVM_STAGE_UPDATE_CALLBACK` | macro / define | [src/standard/stage_update_constants.h:5](../src/standard/stage_update_constants.h#L5) |
| `FVM_StageUpdateEventScope` | class | [src/standard/stage_update.hpp:9](../src/standard/stage_update.hpp#L9) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
