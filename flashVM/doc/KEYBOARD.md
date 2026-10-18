# Host keyboard state and AS1/AS2 Key polling

Version 0.589 adds an explicitly installed polling subset of the Flash Key
namespace. It reuses the existing native method/AVM1 dispatch and does not open
an OS device. [Русский контракт](KEYBOARD.ru.md).

## Installation

Call [`fvm_standard_install`](../include/flashvm/standard.h#L18) first, then `fvm_standard_keyboard_install(context)`.
The second function installs the global `Key` object with `getCode`, `getAscii`,
`isDown`, `isToggled` and BACKSPACE, TAB, ENTER, SHIFT, CONTROL, CAPSLOCK, ESCAPE,
SPACE, PGUP, PGDN, END, HOME, LEFT, UP, RIGHT, DOWN, INSERT, DELETEKEY constants.
The constants use `FVM_KEY_*` virtual-key values, rather than platform scancodes.
v0.590 adds [listener registration/delivery](KEYBOARD_LISTENERS.md).
No constructor, `isAccessible` or security-domain behavior is supplied.

Installation returns [`FVM_OK`](../include/flashvm/flashvm.h#L42) only after the complete namespace is prepared.
A conflicting own global property/accessor/native/script/constructor registration
returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) without replacement. Calling it again on its
unshadowed native methods succeeds without restoring script-edited constants or
attributes. Replacing/shadowing a polling method makes that validation fail.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) leaves globals unchanged; a temporarily created unpublished
object is ordinary collectible garbage. Other C++ exceptions become [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).
No foreign C++ exception escapes this API. It requires a live context with the
ordinary standard library installed and rejects collector/teardown reentry.
The original [`fvm_standard_install`](../include/flashvm/standard.h#L18) does not implicitly install this subset.

## Snapshot API

[`FVM_KeyboardState`](../include/flashvm/keyboard.h#L49) is caller-owned plain native storage. Initialize the entire
structure to zero and set `struct_size = sizeof(FVM_KeyboardState)`. Its fields:

| Field | Contract |
| --- | --- |
| `struct_size` | Exact current structure size; larger/older layouts are rejected. |
| `down` | [`FVM_KEYBOARD_CODE_COUNT`](../include/flashvm/keyboard.h#L7) independent bytes, each 0 or 1; slot zero must remain zero. |
| `last_code` | Latest key-down virtual code, 0..255; zero means no known key. Releases can leave this unchanged. |
| `last_ascii` | English ASCII for the latest transition, 0..[`FVM_KEYBOARD_ASCII_MAX`](../include/flashvm/keyboard.h#L8); zero means absent. Not Unicode text composition. |
| `caps_lock`, `num_lock` | Each exactly 0 or 1; toggle state independent of physical held slots. |

`fvm_input_set_keyboard(context, state)` validates every field before copying.
It returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) for null pointers, wrong layout, invalid slots,
codes or toggles. Failure preserves the entire old state; success copies the
snapshot and retains no caller pointer. It does not infer events or change the
pointer/gesture queue. No allocation, script getter or callback occurs.

`fvm_input_get_keyboard(context, state)` requires initialized exact `struct_size`,
then copies the complete current snapshot. Invalid arguments preserve all output
bytes. Fresh contexts contain zero input and their own exact structure size.

`fvm_input_reset_keyboard(context)` clears held keys, latest codes and toggles,
retaining a valid structure size. It returns [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) for an invalid
context. Use on focus loss; publish actual OS toggles again when input resumes.
It delivers no artificial releases. Set/get/reset are allocation-free and return
only [`FVM_OK`](../include/flashvm/flashvm.h#L42) or [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).

All three functions are owning-thread-only, unsynchronized, and reject teardown,
GC collection/release reentry. Their state contains no managed values or GC roots.
A host must marshal platform-thread input before calling them. A coherent
snapshot prevents partial validation updates; it is not cross-thread atomicity.

## Script methods

| Method | Supported behavior |
| --- | --- |
| `Key.getCode()` | Number from the invoking context's `last_code`, initially/reset zero. Extra arguments ignored. |
| `Key.getAscii()` | Number from `last_ascii`, initially/reset zero. Extra arguments ignored. |
| `Key.isDown(code)` | Boolean for one held virtual slot; simultaneous keys remain independent. |
| `Key.isToggled(code)` | Boolean toggle for CAPSLOCK or numeric Num Lock code [`FVM_KEY_NUMLOCK`](../include/flashvm/keyboard.h#L43); false for other slots. |

The two queries convert their first argument through [`fvm_value_to_number`](../include/flashvm/standard.h#L23);
conversion errors/exceptions propagate. Missing arguments, NaN, infinities,
negative values, values below 1 and values >=256 return false. Accepted fractional
values truncate toward zero. Conversion can execute script: the state is read
after conversion, so a host update inside that conversion is visible. Extra
arguments are ignored. Original-player comparisons of numeric edge cases remain
open; these are the explicit current subset contracts, not full conformance.

Methods read the invoking context, even if the namespace is retained under a
second name. [GC:] The global binding retains Key; a host that retains its object
after replacing that binding must pin/root it normally. Input state itself is
not serialized, and serializing this host-local native namespace currently
returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46). Reinstall explicitly in another context and publish
its own snapshot. No pointer/OS state is transferred through a graph.

## SDL player

The player explicitly installs Key and publishes one complete input snapshot
before each movie tick. [Translation](../frontend/common/keyboard_sdl.h) supports
letters/digits, arrows, navigation, Shift/Control/Alt, lock keys, F1..F12, keypad
digits and English punctuation. Left/right modifiers share a virtual slot while
the SDL modifier mask prevents releasing one from releasing its still-held twin.
English letter case uses Shift XOR Caps Lock; shifted digits/punctuation map to
English ASCII. Other key symbols produce no held slot or key-down code.
Unsupported ASCII translation is zero. This is not an IME, layout-independent
physical-scancode translator, control-character mapping or composed-text API.

The SDL host clears held/codes/toggles on focus loss/background. Escape and Android
Back retain their existing close behavior. Very short presses can finish between
movie ticks; held polling then reads false. HostInput counters still preserve
such taps. Existing HostInput pointer/button/counter fields remain available.
Listener delivery is now explicit through [the v0.590 dispatch/FIFO](KEYBOARD_LISTENERS.md).
Automatic button/clip-event routing remains open.

## Examples and verification

[Commented C89 tutorial](../examples/embedding/keyboard_polling/README.md) demonstrates
setup, simultaneous held slots, polling and reset with ordinary error handling.
`tests/input_keyboard.c` checks isolation, validation, installation conflicts,
GC and unsupported serialization. `tests/jit_keyboard.cpp` checks real AVM1 calls
in interpreter/manual-JIT/automatic-JIT modes with SWF5..8. Allocation-failure
sweeps cover transactional installation and allocation-free snapshot operations.
`frontend/tests/keyboard.c` exercises actual SDL translation and focus/background
handling without opening devices. These tests do not constitute physical-keyboard
or full original-Flash acceptance on every OS.

Reference: Adobe's [ActionScript 2.0 Language Reference](https://homepage.cs.uiowa.edu/~slonnegr/flash/ActionScript2Reference.pdf),
Key chapter (original publisher document, mirrored by the University of Iowa).
Unimplemented security and clip/button event semantics remain explicit roadmap work.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:62](../include/flashvm/keyboard.h#L62) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:64](../include/flashvm/keyboard.h#L64) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:60](../include/flashvm/keyboard.h#L60) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_KEY_NUMLOCK` | macro / define | [include/flashvm/keyboard.h:43](../include/flashvm/keyboard.h#L43) |
| `FVM_KEYBOARD_ASCII_MAX` | macro / define | [include/flashvm/keyboard.h:8](../include/flashvm/keyboard.h#L8) |
| `FVM_KEYBOARD_CODE_COUNT` | macro / define | [include/flashvm/keyboard.h:7](../include/flashvm/keyboard.h#L7) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:49](../include/flashvm/keyboard.h#L49) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:81](../include/flashvm/keyboard.h#L81) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_value_to_number` | function | [include/flashvm/standard.h:23](../include/flashvm/standard.h#L23) |
<!-- source-links:end -->
