# Retained ClipEventKeyPress input — v0.628

[Русский](CLIP_KEYPRESS.ru.md). [`fvm_input_clip_key_code`](../include/flashvm/keyboard.h#L68) converts a copied native
keyboard transition to a SWF selector. [`fvm_movieclip_dispatch_key_press`](../include/flashvm/clip_actions.h#L106) delivers
that exact raw byte through a selected retained MovieClip tree. The SDL player
runs this stage after successful KeyDown records and named methods.

## Wire codes and pure translation

The [Adobe SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf),
CLIPACTIONRECORD and DefineButton2/BUTTONCONDACTION sections, assigns a KeyCode
byte to ClipEventKeyPress and refers to the button key-code table. Special SWF
codes differ from the virtual keys used by Key polling; printable codes use ASCII.
These source references define wire selectors, not complete event-order conformance.

| Key | Native virtual code | SWF selector |
| --- | --- | --- |
| Left / Right | 37 / 39 | 1 / 2 |
| Home / End | 36 / 35 | 3 / 4 |
| Insert / Delete | 45 / 46 | 5 / 6 |
| Backspace / Enter | 8 / 13 | 8 / 13 |
| Up / Down | 38 / 40 | 14 / 15 |
| Page Up / Page Down | 33 / 34 | 16 / 17 |
| Tab / Escape | 9 / 27 | 18 / 19 |
| Printable character | Copied last_ascii | 32–126 |

The public `FVM_CLIP_KEY_*` macros describe the raw domain. Never pass Key.LEFT=37
expecting a Left selector: raw 37 denotes printable percent. The pure mapper takes
`const FVM_KeyboardState *`, pressed=0/1 and a required output pointer. It validates
the complete ABI/held/toggle/code/ASCII snapshot before writing output. Invalid
arguments return INVALID_ARGUMENT and preserve the output. It allocates nothing,
uses no context, publishes nothing and never modifies the input snapshot.

Release, absent latest code, a latest key not held, or unmapped/nonprintable input
returns OK with [`FVM_CLIP_KEY_NONE`](../include/flashvm/clip_actions.h#L24) (zero). Hosts skip that result. On a held press,
special virtual identities take priority; otherwise printable last_ascii is used.
The mapper does not infer a character from virtual letters or apply Shift/Caps a
second time. The SDL adapter already produces English ASCII with those modifiers.
Repeating the same pressed snapshot translates again. No IME, Unicode composition,
keyboard layout autodetection or physical-device acceptance is added.

## Explicit tree delivery

Borrow/pin a same-context native root on the context owner thread. The dispatcher
accepts a raw unsigned byte 0–255 and matches it literally against retained selectors.
Explicit zero/reserved bytes are legal literal queries, preserving existing parser
and selected-record policy; zero is not a wildcard. Larger values are invalid.
It does not call onKeyPress, onKeyDown, Key listeners or any same-name global function,
change state/focus, tick, poll timers, perform hit testing or render.

Source SWF1–5 nodes are traversed without KeyPress delivery; source SWF6–8 and host
version-zero policy deliver matching retained records. Native ambient policy and
[context/profile limits](CLIP_INPUT_METHODS.md) remain unchanged. Hidden/stopped and
unfocused clips are not filtered by this explicit global host route. It does not
implement DefineButton2 conditions or certify original Player focus/visibility order.

The same [prepared tree contract](CLIP_INPUT.md) applies: capture depth-ordered
recipients before callbacks, validate ancestry/removal at each turn, skip removed/
moved branches and defer newborns to the next broadcast. Each recipient prepares
its current selector/code table at its own turn. Matching records run in source
order; a combined KeyPress/KeyDown record ignores its key selector for KeyDown,
but requires an exact selector for KeyPress. No persistent cursor or retry exists.

Capture/sort, ancestry, every retained record/code validation and AVM1 share one
execution-work scope. Numeric exhaustion and cooperative worker stop return ERROR.
Preparation/topology/type/phase/reentry failures return INVALID_ARGUMENT; allocation
failure returns OUT_OF_MEMORY. Actual handler errors/exceptions propagate. Completed
input/script effects survive errors; remaining recipients are skipped.

[GC:] The outer native barrier retains the prepared graph and borrowed code values;
collect is invalid and incremental step does not collect. Same-context recursion
through any input-tree route and context teardown are blocked. Guards unwind on
errors, native throws and allocation failures. Active batches/flags are not graph
state; retained selectors/code and source identity survive ordinary serialization.

The new host KeyPress route grants updateAfterEvent eligibility while executing
matching records. Requests coalesce and survive later errors. The older selected
[`fvm_movieclip_dispatch_clip_actions`](../include/flashvm/clip_actions.h#L58) KeyPress call retains its original policy:
it does not introduce eligibility itself, though an eligible outer scope is inherited.
This distinction is explicit rather than silently changing earlier host contracts.

## SDL order, errors and resets

For a queued supported key-down: publish copied state and Key observers, deliver
scene KeyDown records and eligible onKeyDown methods, then translate the original
queued snapshot and deliver KeyPress if nonzero. A callback's changes to the VM's
current keyboard state do not change which copied key is selected; callbacks still
observe current native state, including earlier explicit changes.

Each tree event is a separate synchronous API call with its own outer budget and
fresh topology capture. A child born during KeyDown may receive the following
KeyPress; it was not retroactively added to the preceding KeyDown batch. No combined
transaction or whole-SDL-transition numeric budget is claimed. Explicit Key listener
registration remains independent of scene methods; the v0.627 focus deduplication
route is preserved.

Key-up and unmapped input have no KeyPress stage. SDL autorepeat key-down messages
still generate it; existing key_presses host stats count only nonrepeat downs.
Escape and Android back remain reserved by the existing close handling, so explicit
C hosts can use selector 19 but the desktop player does not promise an Escape movie
event. Shift/Caps and punctuation retain the current English mapping.

A namespace/KeyDown error or changed reset epoch prevents KeyPress. A KeyPress error
consumes the current transition, preserves committed state and leaves later queued
transitions for the next pump. Callback appends wait for the next entry batch;
recursive pump remains invalid. Focus/background reset cancels old FIFO entries.
Only the separate player test fixture injects messages; production input has no injector.

## Evidence and example

- [C89 mapper](../tests/clip_key_code.c): independent special-code table, all ASCII
  values, release/unheld/absent and invalid snapshot/output preservation.
- [Tree](../tests/clip_keypress.cpp): exact/raw/reserved selectors, mixed event flags,
  SWF5–8, old API separation, no fictional method/global fallback and unchanged state.
- [Mutation](../tests/clip_keypress_mutation.cpp), [budget](../tests/clip_keypress_budget.cpp),
  [OOM](../tests/clip_keypress_allocation.cpp), [errors](../tests/clip_keypress_errors.cpp),
  [workers](../tests/clip_keypress_worker.cpp): removed/deferred clips, hidden/stopped,
  partial effects, allocation sweep, exception/native throws, GC/reentry/teardown,
  host redraw and actual OS-worker cooperative stop.
- [Graph/JIT](../tests/clip_keypress_graph.cpp): source destruction, traced-string GC,
  exact-selector miss/hit and actual interpreter/native entries.
- [Real SDL adapter](../frontend/tests/clip_keypress.c): namespace → record → method →
  KeyPress, focused clip, deliberate native-state mutation, Shift/Caps/repeat/release,
  unmapped F1, append/error/reset behavior and redraw.
- [Actual player](../frontend/tests/clip_keypress.py) with the
  [test-only queue source](../frontend/tests/clip_player.c): 21 downs/20 ups, 19 selected
  KeyPress deliveries, independent ordered special/ASCII oracle, named method ordering,
  wrong-value controls and method-independent KeyPress redraw for SWF5–8 in interpreter/JIT.

The [C89 tutorial](../examples/embedding/clip_actions/README.md) adds explicit Left
snapshot publication, translation 37→1, marker 168 and redraw consumption. These are
Linux synthetic/dummy-video checks, not physical input, visible-device, foreign OS
or full original-player acceptance. All nine roadmap stages remain open.


v0.628 verification: 286 selected Release CTests, 101 whole-core ASan/UBSan CTests
and the strict C89 sanitizer example passed. Leak detection was disabled. A separate
fixture linked with the v0.627 SDL adapter rejected the same SWF8 KeyPress trace
movie. [Stage status and acceptance limits](ROADMAP_STATUS.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CLIP_KEY_NONE` | macro / define | [include/flashvm/clip_actions.h:24](../include/flashvm/clip_actions.h#L24) |
| `fvm_input_clip_key_code` | function | [include/flashvm/keyboard.h:68](../include/flashvm/keyboard.h#L68) |
| `FVM_KeyboardState` | struct | [include/flashvm/keyboard.h:50](../include/flashvm/keyboard.h#L50) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:58](../include/flashvm/clip_actions.h#L58) |
| `fvm_movieclip_dispatch_key_press` | function | [include/flashvm/clip_actions.h:106](../include/flashvm/clip_actions.h#L106) |
<!-- source-links:end -->
