# Primary-pointer Mouse listeners

v0.595 provides an explicit SWF6–8 Mouse listener subset and a C89 host boundary.
A host can supply its primary touch contact, stylus or mouse; no physical mouse,
OS device, permission or window is opened by the VM. [Русский контракт](MOUSE.ru.md).


The SDL player bridge added in v0.596 maps an actual primary finger into these
events and preserves key/pointer order; see [player input](PLAYER_POINTER.md).

## Installation

`fvm_standard_mouse_install(context)` borrows a live, standard-library-installed
context on its owning thread, outside collection, release or destruction. It
atomically prepares Mouse.addListener/removeListener and the _listeners getter.
It does not install Key/Selection, change position or execute a script callback.

[FVM_OK](../include/flashvm/flashvm.h#L42) means successful publication or an intact existing namespace.
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) means an invalid context/state, missing standard library,
or a conflicting global/accessor/callable/method identity. Loaded SWF below
[FVM_MOUSE_MIN_SWF_VERSION](../include/flashvm/mouse.h#L5) (6) returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Host contexts are supported.
[FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) and contained unexpected [FVM_ERROR](../include/flashvm/flashvm.h#L43) preserve the global binding.
Unpublished intermediate objects remain ordinary context-owned garbage.
Installation respects case-insensitive global lookup in SWF6 and case-sensitive
lookup in SWF7–8/host contexts. Standard installation alone does not opt in.
Namespace methods/getter also reject SWF1–5 if a host installed before loading.

## Native host contract

- `fvm_input_add_mouse_listener(context, listener)` borrows a non-null same-context
  object and retains its graph once. Duplicate identity returns [FVM_OK](../include/flashvm/flashvm.h#L42) without
  moving it. [FVM_MOUSE_MAX_LISTENERS](../include/flashvm/mouse.h#L6) (1024) gives [FVM_ERROR](../include/flashvm/flashvm.h#L43) at saturation; allocation
  failure gives [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) without registration. No callback runs.
- `fvm_input_remove_mouse_listener(context, listener, removed)` requires a valid
  same-context object and non-null int output. [FVM_OK](../include/flashvm/flashvm.h#L42) stores 1 if removed, 0 if
  absent; invalid arguments/state return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) without touching the
  output. Removal allocates nothing and executes no script.
- `fvm_input_dispatch_mouse(context, type, world_x, world_y)` synchronously delivers
  [FVM_MOUSE_EVENT_DOWN](../include/flashvm/mouse.h#L14), [FVM_MOUSE_EVENT_MOVE](../include/flashvm/mouse.h#L13) or [FVM_MOUSE_EVENT_UP](../include/flashvm/mouse.h#L15). Coordinates are
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

Cursor hide/show, wheel, direct MovieClip/Button/ClipActions dispatch, automatic
focus/hit routing, raw multitouch/gesture recognition, security-origin policy,
original-player edge ordering and native device/endurance acceptance remain open.
Basic listener signatures follow the [Adobe AS2 reference mirror](https://open-flash.github.io/mirrors/as2-language-reference/Mouse.html).
Snapshot mutation, limits and explicit host mapping are documented subset policies,
not full Flash compatibility. No roadmap stage closes; see [status](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_add_mouse_listener` | function | [include/flashvm/mouse.h:18](../include/flashvm/mouse.h#L18) |
| `fvm_input_dispatch` | function | [include/flashvm/input.h:82](../include/flashvm/input.h#L82) |
| `fvm_input_dispatch_mouse` | function | [include/flashvm/mouse.h:25](../include/flashvm/mouse.h#L25) |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:75](../include/flashvm/input.h#L75) |
| `fvm_input_remove_mouse_listener` | function | [include/flashvm/mouse.h:20](../include/flashvm/mouse.h#L20) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:72](../include/flashvm/input.h#L72) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOUSE_EVENT_DOWN` | enum constant | [include/flashvm/mouse.h:14](../include/flashvm/mouse.h#L14) |
| `FVM_MOUSE_EVENT_MOVE` | enum constant | [include/flashvm/mouse.h:13](../include/flashvm/mouse.h#L13) |
| `FVM_MOUSE_EVENT_UP` | enum constant | [include/flashvm/mouse.h:15](../include/flashvm/mouse.h#L15) |
| `FVM_MOUSE_MAX_LISTENERS` | macro / define | [include/flashvm/mouse.h:6](../include/flashvm/mouse.h#L6) |
| `FVM_MOUSE_MIN_SWF_VERSION` | macro / define | [include/flashvm/mouse.h:5](../include/flashvm/mouse.h#L5) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_mouse_install` | function | [include/flashvm/mouse.h:29](../include/flashvm/mouse.h#L29) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
