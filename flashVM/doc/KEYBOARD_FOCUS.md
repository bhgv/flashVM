# Explicit host keyboard focus

v0.591 adds a C89 host selection for a native MovieClip. The selected clip receives
`onKeyDown` / `onKeyUp` through the same rooted, receiver-bound method dispatch as
[Key listeners](KEYBOARD_LISTENERS.md). [Русский контракт](KEYBOARD_FOCUS.ru.md).

This is an embedding policy: it does not install `Selection`, implement Flash
focus eligibility, synthesize mouse/Tab selection, deliver focus-change callbacks,
or decode SWF ClipActions. The SDL player does not automatically select a clip.
A host can use this API to route physical input into its chosen movie/clip while
keeping its own UI responsible for selection. It deliberately ignores script
`focusEnabled`, `tabEnabled`, visibility and `_focusrect`; this low-level override
must not be presented as original-player `Selection.setFocus` compatibility.

## fvm_input_set_keyboard_focus

`fvm_input_set_keyboard_focus(context, clip)` selects one native MovieClip in a
live context. `context` is a borrowed non-null VM owner. `clip` is either null
(clear the selection) or a borrowed live object owned by exactly that context.
A standalone native root is valid; class prototypes, ordinary objects and clips
whose native state or physical ancestry is detached are rejected. Validation
uses private scene storage, ignoring replaceable script `_parent` properties.

Success returns [`FVM_OK`](../include/flashvm/flashvm.h#L42) and replaces the previous selection atomically. The
context retains a strong GC root to the new clip and its reachable scene graph;
no extra pin is needed while it remains selected. The previous selection loses
only this root, not other globals, pins or listener registrations. No script
runs, no keyboard state changes and no event is sent. Null clearing performs no
allocation. Non-null validation may allocate temporary native lookup strings. The native
ancestry walk is bounded by [`FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS`](../src/standard/movieclip_constants.h#L78) (256 visited
nodes, including the selected clip).

[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) means invalid context, wrong ownership/type/prototype,
detachment or malformed private state. [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) means validation
allocation failed. [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) covers excessive/cyclic native ancestry or an
unexpected contained C++ exception. Errors preserve the previous selection.
There is no transfer of native userdata ownership and no OS device interaction.

## fvm_input_get_keyboard_focus

`fvm_input_get_keyboard_focus(context, out)` requires a live borrowed context and
a non-null caller-owned `FVM_Object **out`. Success returns [`FVM_OK`](../include/flashvm/flashvm.h#L42), storing the
borrowed clip pointer, or null when no clip is selected. It does not create a pin:
retain an ordinary GC root if keeping this pointer after replacing/clearing focus.

A selection removed from the native scene, including through a removed ancestor,
is cleared lazily by this query or the next keyboard dispatch. Success then
stores null. Until that check, clear/replacement or destruction, the context may
still retain the detached graph. Merely hiding/reparenting a live clip does not
clear focus. The query invokes no script getters and sends no callbacks.

[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) covers invalid context/output or malformed native state;
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) covers validation allocation; [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) covers excessive
ancestry or an unexpected contained C++ exception. Errors preserve output and
the selection. An empty query performs no allocation.

## Delivery and lifetime

[`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90) validates the supplied snapshot, checks the current
selection for detachment, and prepares a strongly rooted recipient snapshot:
selected clip first, then Key listeners in registration order. Only after this
preparation succeeds is the keyboard state published. Each receives zero arguments
and its own object as `this`; use Key polling or [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L71) for codes.
A clip also explicitly registered as a Key listener receives both deliveries;
the two channels are independent and are not deduplicated. A selected clip is
not implicitly added to Key._listeners or its registration limit.

The clip-first order is FlashVM's documented host policy; original-player order
has not been verified. Focus changes during a callback affect the next event.
The current recipient snapshot remains rooted even if a callback changes focus,
removes later listeners and triggers GC. Handler lookup occurs when that recipient
is delivered, so earlier callbacks can replace later handlers. Missing/noncallable
handlers are ignored; real getter/handler errors stop delivery and propagate.
Already published state and callback effects remain committed. Preparation failure
preserves keyboard state; detecting detachment may already clear stale focus.
Nested keyboard dispatch remains rejected.

Selection is per context, on the VM-owning thread, with no synchronization. Both
APIs reject teardown and GC collection/release, but may be used inside normal
keyboard callbacks. Context destruction releases the root and remains blocked
while keyboard notification is active. Focus, physical state and listener
registrations are host-local, are not serialized, and are not automatically
cleared by [`fvm_load_memory`](../include/flashvm/flashvm.h#L72), Key replacement, or [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L73).
Clear focus explicitly when unloading a host movie or transferring keyboard
ownership to another UI. OS focus loss resets physical state separately.

## Example and verification

[Runnable C89 tutorial](../examples/embedding/keyboard_listeners/README.md) compares
ordinary listener routing and explicit native clip selection without a window.
It calls [`fvm_movieclip_create`](../include/flashvm/movieclip.h#L36) after standard installation, then immediately
selects the returned borrowed clip before allocating handler setup. This protects
the clip during construction; context destruction covers every error path.

`tests/keyboard_focus.cpp` checks ownership/type/prototype rejection, GC retention,
focus replacement inside a collecting callback, listener snapshot retention,
independent duplicate channels, real handler failure, native ancestor removal and
lazy clearing. `tests/jit_keyboard_listeners.cpp` executes real script callbacks
with clip receivers on SWF6–8 in interpreter, explicit JIT and automatic JIT modes.
`tests/keyboard_dispatch_allocation.cpp` checks validation output preservation,
preparation failure and allocation-free null clearing. These are host/runtime
fixtures, not physical keyboard acceptance or full Flash focus conformance.

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L101).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:71](../include/flashvm/keyboard.h#L71) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:86](../include/flashvm/keyboard.h#L86) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:73](../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_movieclip_create` | function | [include/flashvm/movieclip.h:36](../include/flashvm/movieclip.h#L36) |
| `FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:78](../src/standard/movieclip_constants.h#L78) |
| `FVM_Object` | struct | [src/core/runtime.hpp:122](../src/core/runtime.hpp#L122) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../include/flashvm/keyboard.h#L101) |
<!-- source-links:end -->
