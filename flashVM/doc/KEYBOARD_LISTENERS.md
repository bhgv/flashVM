# Key listeners and ordered keyboard delivery

v0.590 extends the explicit Key namespace with registration and synchronous
notifications. [Polling/snapshot ABI](KEYBOARD.md) / [Русский контракт](KEYBOARD_LISTENERS.ru.md).
It advances input support; automatic MovieClip/button key events, security origins,
IME/text composition and complete original-player conformance remain open.

## Native API

All functions below use the VM-owning thread. They are not synchronized and must
not run during context teardown or GC collection/release. Registration does not
open an OS device, change keyboard state or execute a handler. The C API does not
require Key or the standard library to be installed; the script bridge does.

### fvm_input_add_key_listener

`fvm_input_add_key_listener(context, listener)` retains a unique same-context
object by identity. `context` and `listener` are borrowed non-null live owners;
foreign-context objects are rejected. No managed capture or native userdata is
copied. Duplicate registration returns [`FVM_OK`](../include/flashvm/flashvm.h#L42) without allocation or reordering.
A successful new registration is appended in delivery order and is a strong GC
root until removal or context destruction, even if script deletes the Key global
or the listener's last ordinary reference.

Returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) for invalid context/object/ownership;
[`FVM_ERROR`](../include/flashvm/flashvm.h#L43) at [`FVM_KEYBOARD_MAX_LISTENERS`](../include/flashvm/keyboard.h#L6) unique registrations (1024), or for an
unexpected contained C++ exception; [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) if native vector growth
fails. Failure preserves the previous registration list. Callers retaining a
removed object must protect it with an ordinary GC root/pin.

### fvm_input_remove_key_listener

`fvm_input_remove_key_listener(context, listener, removed)` removes by identity.
Arguments require a live context, non-null same-context object and non-null
caller-owned `int *removed`. Invalid input returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) and
preserves both the list and output. Success returns [`FVM_OK`](../include/flashvm/flashvm.h#L42), setting `*removed`
to one when found or zero when absent. Removal does not allocate, shift remaining
registration order semantically, reset keyboard state or notify a handler.

Removal during a delivery changes future event snapshots; a recipient already
in the current snapshot still receives that event. Remove/re-add moves an object
to the end of the next event's order. No finalization is forced by removal.

### fvm_input_dispatch_keyboard

`fvm_input_dispatch_keyboard(context, state, pressed)` borrows a complete
post-transition [`FVM_KeyboardState`](../include/flashvm/keyboard.h#L50) and a Boolean `pressed` (exactly 0 or 1).
It validates every snapshot field using the same rules as [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L61).
The host supplies consistent physical state, latest codes and lock toggles; this
function does not infer them from the phase or query the OS. Repeats may be sent
explicitly as another press. The snapshot is copied; its pointer is never retained.

Since v0.591 preparation puts an explicitly selected native clip first, followed
by the registration list, into a temporary rooted value vector. See
[host selection, independent channels and detachment](KEYBOARD_FOCUS.md#delivery-and-lifetime).
Only after successful preparation does it publish the new polling state and
resolve each recipient's `onKeyDown` or `onKeyUp` method, with zero positional
arguments and the listener as `this`. Registration order is deterministic.
Adds/removes affect the next event's snapshot; method replacements/getters are
resolved at that recipient's delivery time. Missing/noncallable handlers are
ignored; inherited methods, accessors and loaded `__resolve` follow the existing
method dispatch rules. A missing object handler does **not** fall back to a
same-named global native function.

An actual getter/handler failure propagates, including [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) returned
by callable native code. It stops remaining recipients. Already committed state,
callbacks and registration edits are retained, not rolled back. Handler return
values are ignored; VM result/exception status is not ignored. Do not blindly
retry a failed event: earlier recipients could run twice. Pending exceptions
remain owned by the normal exception APIs.

Invalid pointers/layout/state/phase or recursive keyboard dispatch return
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) before state changes. Preparation OOM returns
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) with old state unchanged. OOM during later method lookup or
execution propagates after publication. Unexpected C++ exceptions are contained
as [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). There are no foreign exceptions across the C ABI.

[GC:] The context marks all active registrations. The temporary recipient vector
also roots removed recipients until the current event ends; owner/receiver,
arguments and getter results use the existing rooted dispatcher. Script handlers
retain interpreter/JIT GC rules. Ordinary native handlers keep the usual native
collector barrier. Context destruction is refused while delivery is active;
callers must never destroy its runtime or presentation objects from a handler.
A scope guard clears the delivery flag and unlinks temporary roots on every exit.
Polling set/reset calls and registration edits remain allowed inside a handler;
subsequent handlers then see that explicitly updated polling state.

## Script interface

Call [`fvm_standard_install`](../include/flashvm/standard.h#L18), then [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L85) as before.
Installation now also prepares these methods and the inspection getter atomically;
reinstallation validates their unshadowed native identities. It neither clears
nor duplicates existing registrations. Explicit installation exposes the subset;
original-player version availability/security gating is not fully implemented.

| Member | Current contract |
| --- | --- |
| `Key.addListener(object)` | Register once; returns Undefined. Missing/non-object arguments are ignored without coercion. Native registration errors propagate. |
| `Key.removeListener(object)` | Return Boolean removed; missing/non-object arguments return false. |
| `Key._listeners` | Read-only property returning a fresh Array snapshot in registration order. |

The snapshot Array contains the original listener references and uses the retained
builtin Array prototype, without calling a script-replaced Array constructor or
prototype getter. Root it normally if retained by native code across collection.
Editing that Array does not add/remove registrations; later getter reads return
new snapshots. Stable Array identity and original-player mutation behavior are
not claimed. Assigning the getter property is ignored and deletion is blocked by
its default attributes. Getter allocation failures use ordinary VM error handling.

Registrations belong to the context, not a particular Key object/movie. Reloading
a movie or replacing Key does not unregister callbacks. The host should explicitly
remove obsolete registrations. Neither physical input state nor registrations
transfer with serialized graphs; imported listeners must be registered in their
receiving context. Serializing the native Key namespace remains unsupported.

## SDL event queue

The main-thread window adapter copies supported key transitions into a native
FIFO of `FVM_DESKTOP_PENDING_KEYS` (128) complete snapshots. It retains presses,
releases and repeats even when all occur between movie ticks. Unsupported SDL
key symbols do not enqueue notifications. No VM code runs in SDL event collection.

`fvm_desktop_window_pump_keyboard` runs on the owning thread before the next tick,
then the player publishes final HostInput/polling state and advances the movie.
The pump processes only its entry batch; recursively pumping is invalid. Focus
loss/background cancels pending transitions and clears held/codes/toggles without
inventing releases. Escape/Android Back retain their close behavior. The FIFO
retains no VM pointers or roots and is outside the VM allocation budget.

Overflow latches [`FVM_ERROR`](../include/flashvm/flashvm.h#L43); the player reports an input failure rather than
silently dropping it. Pump removes the current event before dispatch; any error
consumes that event and retains later queued ones for a host that chooses to
continue. The normal player reports the error and closes the current movie.
Focus cancellation does not conceal an earlier overflow error. Callbacks must
not destroy/reenter the host presentation lifecycle.

## Examples and verification

- [Native C89 listener example](../examples/embedding/keyboard_listeners/README.md): setup, strong registration, press/release and ownership cleanup.
- [Interactive SWF 02](../examples/player/animation_audio/README.md): a movie-owned observer registers once across timeline loops. Down events grow the green marker vertically; up events shift it down. A very short tap updates both counts even when held polling misses it.
- Core regressions cover rooted remove/add/GC, duplicates, receiver binding, inherited single-evaluation getters, no global fallback, ignored noncallable members, callable failures, exceptions and nested dispatch rejection.
- Native allocation-failure checks distinguish preparation from committed delivery and verify allocation-free duplicate/remove paths.
- Actual script registration/callback bodies run on SWF6..8 in interpreter/manual-JIT/automatic-JIT modes.
- The shipped SWF runs three loops in interpreter/JIT with one listener and matching press/release counters; native SDL translation/FIFO tests cover a whole tap between ticks and explicit overflow without opening devices.

These are bounded implementation tests, not physical keyboard/foreign-OS
acceptance or original-player mutation semantics certification. The primary
interface reference is Adobe's [AS2 Language Reference](https://homepage.cs.uiowa.edu/~slonnegr/flash/ActionScript2Reference.pdf), Key chapter.


[Explicit host keyboard focus / Явный выбор клипа](KEYBOARD_FOCUS.md) ([RU](KEYBOARD_FOCUS.ru.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L72), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L75).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:67](../include/flashvm/keyboard.h#L67) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:79](../include/flashvm/keyboard.h#L79) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:75](../include/flashvm/keyboard.h#L75) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:69](../include/flashvm/keyboard.h#L69) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:61](../include/flashvm/keyboard.h#L61) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:72](../include/flashvm/keyboard.h#L72) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_KEYBOARD_MAX_LISTENERS` | macro / define | [include/flashvm/keyboard.h:6](../include/flashvm/keyboard.h#L6) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:50](../include/flashvm/keyboard.h#L50) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:85](../include/flashvm/keyboard.h#L85) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
