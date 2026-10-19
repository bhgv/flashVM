# Primary-pointer Mouse listeners

v0.595 provides an explicit SWF6–8 Mouse listener subset and a C89 host boundary.
A host can supply its primary touch contact, stylus or mouse; no physical mouse,
OS device, permission or window is opened by the VM. [Русский контракт](MOUSE.ru.md).


The SDL player bridge added in v0.596 maps an actual primary finger into these
events and preserves key/pointer order; see [player input](PLAYER_POINTER.md).

## Installation

`fvm_standard_mouse_install(context)` borrows a live, standard-library-installed
context on its owning thread, outside collection, release or destruction. It
atomically prepares Mouse.hide/show, addListener/removeListener and the _listeners getter.
It does not install Key/Selection, change position or execute a script callback.

[FVM_OK](../include/flashvm/flashvm.h#L42) means successful publication or an intact existing namespace.
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) means an invalid context/state, missing standard library,
or a conflicting global/accessor/callable/method identity. Loaded SWF below
[FVM_MOUSE_CURSOR_MIN_SWF_VERSION](../include/flashvm/mouse.h#L5) (5) returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Host contexts are supported.
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) and contained unexpected [FVM_ERROR](../include/flashvm/flashvm.h#L43) preserve the global binding.
Unpublished intermediate objects remain ordinary context-owned garbage.
Installation respects case-insensitive global lookup in SWF5–6 and case-sensitive
lookup in SWF7–8/host contexts. Standard installation alone does not opt in.
Listener methods/getter reject SWF1–5; hide/show reject SWF1–4, including namespaces installed before loading.

## Native host contract

- `fvm_input_add_mouse_listener(context, listener)` borrows a non-null same-context
  object and retains its graph once. Duplicate identity returns [FVM_OK](../include/flashvm/flashvm.h#L42) without
  moving it. [FVM_MOUSE_MAX_LISTENERS](../include/flashvm/mouse.h#L7) (1024) gives [FVM_ERROR](../include/flashvm/flashvm.h#L43) at saturation; allocation
  failure gives [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) without registration. No callback runs.
- `fvm_input_remove_mouse_listener(context, listener, removed)` requires a valid
  same-context object and non-null int output. [FVM_OK](../include/flashvm/flashvm.h#L42) stores 1 if removed, 0 if
  absent; invalid arguments/state return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) without touching the
  output. Removal allocates nothing and executes no script.
- `fvm_input_dispatch_mouse(context, type, world_x, world_y)` synchronously delivers
  [FVM_MOUSE_EVENT_DOWN](../include/flashvm/mouse.h#L15), [FVM_MOUSE_EVENT_MOVE](../include/flashvm/mouse.h#L14) or [FVM_MOUSE_EVENT_UP](../include/flashvm/mouse.h#L16). Coordinates are
  finite stage/world pixels, before MovieClip inverse transforms. Invalid type,
  NaN/infinity, invalid state or nested Mouse dispatch gives [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45)
  without position change or notification. The host chooses its primary contact;
  the VM does not infer a pressed state, suppress duplicates or recognize gestures.
- `fvm_input_get_pointer(context, x, y)` copies the current position without
  allocating or querying a device. Its existing non-null/distinct output contract
  applies. MovieClip._xmouse/_ymouse and AVM1 pointer instructions use this state.
  [`fvm_input_set_pointer`](../include/flashvm/input.h#L72) remains a silent publication operation.

All functions run on the context owner thread, outside collection/release/teardown.
Native C registration/delivery is independent of script namespace installation.
No managed object ownership transfers to the host; explicit removal or context
teardown releases the registration. Borrowed callback code/userdata must outlive
all registrations/in-flight deliveries. Do not destroy a context from a callback.

## Delivery

Dispatch first copies/roots all registered receiver identities. Preparation OOM
preserves pointer and drag state. It then publishes both coordinates, applies the
existing active native drag update and calls onMouseDown/onMouseMove/onMouseUp
with no arguments on each snapshot receiver in registration order. Script can
read the new position during the callback. No hit target or focus is required.

Ordinary versioned/inherited/accessor/script/native method lookup applies once
per delivery, preserving the receiver. Missing/noncallable handlers are ignored;
global functions of the same name are not fallbacks. Handler results are ignored.
Script getters/functions may collect; native callbacks retain their ordinary GC
barrier unless an internal trusted entry explicitly provides its own roots.
Snapshot receivers remain rooted when earlier callbacks remove them and collect.
Adding/removing subscriptions affects later events; replacing a pending handler
affects its delivery. Recursive Mouse dispatch is rejected, while independent
host operations remain governed by their existing contracts.

A real drag/getter/handler error stops subsequent delivery and propagates its
status/VM exception. [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) after position publication keeps the new
position, as do [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46)/FVM_INVALID_ARGUMENT returned by a real handler.
Previous side effects are not rolled back. Callback changes to position remain
current; dispatch does not republish its old position afterwards. All temporary
roots and the recursion guard are restored on success, status error and C++ unwind.

Legacy gesture [`fvm_input_dispatch`](../include/flashvm/input.h#L82) and its C callback are independent: they do
not automatically invoke Mouse listeners, and Mouse dispatch does not invent a
gesture callback. Hosts should choose the intended route without double delivery.

## Script methods and inspection

`Mouse.addListener(object)` returns undefined; absent/null/undefined/primitive
arguments are ignored. Same-context validation and native allocation/limit errors
propagate. `Mouse.removeListener(object)` returns true only when removed, otherwise
false for absent/invalid script inputs. The enclosing call may allocate argument
storage even though native removal itself does not.

`Mouse._listeners` is a non-enumerable, non-deletable getter-only property returning
a new native Array with the current identities. Replacing Array does not execute
a user constructor here. Changing the inspection Array does not edit registrations.
Root a retained result before collecting/allocating operations as appropriate to
its normal borrowed managed-value contract. Getter OOM preserves registrations.
This is an inspection snapshot, not the mutable original-player broadcaster Array.

Strong registrations are context-local, independent of Key/Selection and public
Mouse namespace replacement. They are unsynchronized and are not serialized;
transporting a listener object does not subscribe it in a receiving context.
The native namespace methods have no portable callback codec.

## Scope and validation

[The C89 tutorial](../examples/embedding/mouse_listeners/README.md) demonstrates
one primary contact without an OS device. `tests/mouse_listeners.cpp` covers
identity, snapshot mutation/GC, recursive delivery/teardown guards, active drag,
errors, invalid coordinates, ignored handlers and independent contexts.
`tests/mouse_allocation.cpp` sweeps installation and pre/post-publication OOM.
`tests/jit_mouse.cpp` exercises SWF6–8 script registration and actual event handlers
in interpreter/manual/automatic JIT, collection and a real AVM1 handler exception.

Automatic cursor SDK application, direct MovieClip/Button/ClipActions dispatch, automatic
focus/hit routing, raw multitouch/gesture recognition, security-origin policy,
original-player edge ordering and native device/endurance acceptance remain open.
Basic listener signatures follow the [Adobe AS2 reference mirror](https://open-flash.github.io/mirrors/as2-language-reference/Mouse.html).
Snapshot mutation, limits and explicit host mapping are documented subset policies,
not full Flash compatibility. No roadmap stage closes; see [status](ROADMAP_STATUS.md).

## Wheel events

v0.605 adds [fvm_input_dispatch_mouse_wheel](../include/flashvm/mouse.h#L33)(context, delta, world_x, world_y).
`context` is a borrowed live VM owner, outside collection/release/teardown;
`delta` is a finite Number measured in **host-normalized scroll lines**: positive
up, negative down. Fractional values and explicit zero are preserved. `world_x`
and `world_y` are finite world/stage pixels captured for this event, not screen,
widget or device coordinates. All calls remain on the context owner thread.

The function first snapshots/roots the same Mouse registrations used by pointer
notifications. It then publishes position, updates native drag, and calls each
optional onMouseWheel(delta) in registration order. There is exactly one numeric
argument, with no automatic scrollTarget, hit testing, keyboard-focus change,
button transition, content scrolling or OS device query. Missing/noncallable
handlers are ignored without global fallback. Native delivery does not require
a script Mouse namespace; the existing SWF6–8 namespace registers the same list.

[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) covers invalid context/lifecycle, NaN/infinite delta or
coordinates and nested Mouse delivery. Pointer and wheel notifications share a
recursion guard: neither can recursively dispatch the other. Invalid input and
snapshot OOM preserve coordinates/drag and invoke no handler. After publication,
real drag/getter/handler status or VM exception propagates unchanged, stops later
receivers, and retains committed effects. Allocating optional lookup can also
return [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) after publication. [FVM_ERROR](../include/flashvm/flashvm.h#L43) contains unexpected native
exceptions. All temporary roots/guards unwind on every exit. A callback may
mutate subscriptions and collect; the rooted entry recipients survive removal.
Changes affect later events; pending receivers use their current handler lookup.
Do not destroy the context from a handler (delivery prevents teardown).

The host converts its native units before calling: wheel detents, lines, pixels
and high-resolution device counts are different protocols. Do not pass a pixel
scroll as line delta without an explicit policy. The VM never rounds a delta,
accumulates residual movement, synthesizes MouseMove, or adds a target argument.
See the [C89 example](../examples/embedding/mouse_wheel/README.md).

The [Adobe AS2 reference copy](https://open-flash.github.io/mirrors/as2-language-reference/Mouse.html)
describes onMouseWheel's line delta and optional target; classic wheel support
was Windows-specific. FlashVM exposes explicit platform-neutral host delivery.
This is bounded delta-only support, not acceptance of target picking, historical
Windows event order or native wheel/trackpad/device behavior on every OS.

## Cursor visibility request

v0.608 exposes a context-local requested visibility, initially 1. The VM does not
own a native cursor. Hosts without a physical pointer can retain this request
without creating a mouse or cursor. Input coordinates, button/focus state, pending
FIFO entries, subscriptions and script execution are independent of visibility.

[fvm_input_get_cursor_visible](../include/flashvm/mouse.h#L38)(context, visible_output) requires a live context on
its owner thread outside collector/release/destruction and a non-null writable
int output. [FVM_OK](../include/flashvm/flashvm.h#L42) writes 0/1; [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) preserves output. It queries
the VM request, not an OS cursor or proof that the display applied it.

[fvm_input_set_cursor_visible](../include/flashvm/mouse.h#L43)(context, visible, previous_output) accepts only
visible=0 or 1 and requires a non-null writable previous int. [FVM_OK](../include/flashvm/flashvm.h#L42) publishes
the new request and writes the old 0/1 request, including repeated hide/show.
Invalid context/lifecycle/value/output returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) preserving
both request and output. Both functions allocate nothing, run no script/host
callback and cannot produce OOM or an OS error. They may be called during Mouse
callbacks on the same thread; there is no recursive event delivery. The scalar
has no managed edges or GC pin requirement. State persists for the context,
including movie loads; hosts can explicitly request visible when replacing a movie.

A native host polls after script/input execution and applies the request to its
own window using its SDK, reporting SDK errors separately. Applying a request
does not acknowledge physical scanout. A global native cursor API must not be
silently shared across independent windows/contexts without an owner policy.
Script hide/show are described below; native SDK application is documented in the frontend guide.

## Script hide and show

v0.609 adds Mouse.hide() and Mouse.show() to explicit standard Mouse installation.
Both ignore supplied arguments without coercion, set the context's requested
visibility, and return a **Number containing the previous 0/1 request**. Thus
hide/hide/show/show from the default request returns 1/0/0/1. There is no callback,
OS call, input/focus mutation or allocation in the transition itself; ordinary
method lookup/execution still follows its own allocation/error/GC rules.

Install requires standard objects and supports SWF5–8/host contexts. Below SWF5,
installation and calls through preinstalled cursor methods return UNSUPPORTED
without changing visibility. The full native method table remains stable across
movie loads: addListener/removeListener/_listeners members are present in SWF5,
but their callbacks reject versions below SWF6. This preserves idempotent namespace
identity and never silently enables SWF6 listeners in SWF5; exact historical
member-presence reflection remains unaccepted. Cursor methods have their own
[FVM_MOUSE_CURSOR_MIN_SWF_VERSION](../include/flashvm/mouse.h#L5) (5), separate from listener version 6.

New installation atomically includes hide/show with listener methods and preserves
conflicting bindings on OOM/error. Idempotency now verifies these methods too;
a shadowed/replaced hide/show is not overwritten. Versioned property case rules
apply. Results represent the old VM request, not an OS acknowledgement or whether
a physical mouse exists. A stolen method still operates on its invoking context;
no native window pointer or userdata is retained.

[Adobe's AS2 reference copy](https://open-flash.github.io/mirrors/as2-language-reference/Mouse.html)
describes previous-state numeric returns and visibility from Flash Player 5.
The test suite covers actual SWF5–8 interpreter/manual/automatic JIT calls,
ignored argument coercion traps, repeated values, shadow conflicts, preloaded
version guards and GC. Native window application and foreign/physical runtime
acceptance are separate from these script semantics.

## Native cursor hosts

v0.610 SDL2/GTK3/Qt5 tutorials poll the request on the native VM owner before
painting; --hide-cursor configures a startup request. SDL saves/restores its
global original visibility under a single-window owner policy. GTK applies a
same-display blank/inherited cursor to its realized drawing-area window and owns
its blank descriptor until teardown. Qt applies a local widget override and
restores inheritance. The common colored marker remains content, not the native
cursor. See [frontend API and lifetime](../examples/frontends/README.md)
and individual SDK READMEs. Native setters with no status report issuance only.
Software SDK checks do not certify a physical cursor, device or foreign OS.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_add_mouse_listener` | function | [include/flashvm/mouse.h:19](../include/flashvm/mouse.h#L19) |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:26](../include/flashvm/mouse.h#L26) |
| `fvm_input_dispatch_mouse_wheel` | function | [include/flashvm/mouse.h:33](../include/flashvm/mouse.h#L33) |
| `fvm_input_get_cursor_visible` | function | [include/flashvm/mouse.h:38](../include/flashvm/mouse.h#L38) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../include/flashvm/input.h#L75) |
| `fvm_input_remove_mouse_listener` | function | [include/flashvm/mouse.h:21](../include/flashvm/mouse.h#L21) |
| `fvm_input_set_cursor_visible` | function | [include/flashvm/mouse.h:43](../include/flashvm/mouse.h#L43) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOUSE_CURSOR_MIN_SWF_VERSION` | macro / define | [include/flashvm/mouse.h:5](../include/flashvm/mouse.h#L5) |
| `FVM_MOUSE_EVENT_DOWN` | enum constant | [include/flashvm/mouse.h:15](../include/flashvm/mouse.h#L15) |
| `FVM_MOUSE_EVENT_MOVE` | enum constant | [include/flashvm/mouse.h:14](../include/flashvm/mouse.h#L14) |
| `FVM_MOUSE_EVENT_UP` | enum constant | [include/flashvm/mouse.h:16](../include/flashvm/mouse.h#L16) |
| `FVM_MOUSE_MAX_LISTENERS` | macro / define | [include/flashvm/mouse.h:7](../include/flashvm/mouse.h#L7) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:47](../include/flashvm/mouse.h#L47) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
