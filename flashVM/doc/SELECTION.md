# Selection MovieClip subset

v0.592 adds explicit Selection.getFocus/setFocus for native MovieClips in host
contexts and SWF6–8. [Русский контракт](SELECTION.ru.md). It shares the
[host keyboard focus root](KEYBOARD_FOCUS.md) and existing display-path resolver.
This is a bounded implementation, not complete Flash Selection compatibility.

## Installation

`fvm_standard_selection_install(context)` requires a borrowed non-null live
context with standard objects already installed by [`fvm_standard_install`](../include/flashvm/standard.h#L18).
Use the VM-owning thread, outside context teardown or GC collection/release.
It synchronously prepares a namespace containing only getFocus and setFocus,
then publishes the complete global Selection binding. It executes no script,
changes no focus and opens no devices. Key installation is independent.

Returns [`FVM_OK`](../include/flashvm/flashvm.h#L42) for successful publication or an intact existing installation;
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) for invalid context/state, missing standard library or a
conflicting namespace/callable/global descriptor; [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46) for a loaded
SWF below [`FVM_SELECTION_MIN_SWF_VERSION`](../include/flashvm/keyboard.h#L5) (6); [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) for allocation
failure; [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) for an unexpected contained C++ exception. Every preparation
failure preserves globals and the old focus. Temporary unpublished allocations
remain ordinary context-owned garbage. Idempotency checks native identities,
trusted bridge flags and property shadowing, not arbitrary script attributes.

SWF6 global names use existing case-insensitive lookup; SWF7–8/host names are
case-sensitive. A conflicting `selection` therefore blocks installation only
under SWF6. Intact methods are non-enumerable. Standard installation alone does
not opt into this subset. The SDL player installs it for SWF6–8 and keeps older
movies playable without this namespace. If a host installs before loading and
later loads SWF1–5, both methods return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), preserving focus;
the historical text-field-only Selection contract is not implemented here.

## setFocus

`Selection.setFocus(newFocus)` returns a Boolean; zero arguments, null and
undefined clear focus and return true. Extra arguments are ignored. A non-null
argument may be a same-context native MovieClip object or a counted path string.
Dot/slash paths use the active AVM1 target, falling back to its base activation
clip when no target exists; `_root`/`_level0`, counted Unicode names, versioned
property lookup and `_lockroot` use the existing resolver. An ordinary C call
without an active movie activation cannot resolve a path: pass the object instead.
No arbitrary object/number-to-string coercion is performed.

A clip is eligible when its current focusEnabled converts to true using existing
version-aware AVM1 Boolean conversion, or it has a callable onDragOut, onDragOver,
onPress, onRelease, onReleaseOutside, onRollOut or onRollOver member. The members
are read in that order, stopping after the first callable. Inherited properties,
accessors and loaded `__resolve` use ordinary method/property lookup. Native
method entries are obtained through their first-class function wrappers. A
noncallable member does not make a clip eligible. tabEnabled, visibility and
_focusrect do not participate in this programmatic selection policy. Eligibility
is checked only during the attempt; later flag/handler changes do not automatically
clear an existing selection.

Missing paths, unsupported scalar/object types, class prototypes, ineligible or
detached clips return false without publishing the candidate. Lookup/getter side
effects are not rolled back. Private native
attachment is checked before eligibility and again before publication, since a
getter may remove the candidate. [`FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS`](../src/standard/movieclip_constants.h#L66) bounds that
native walk; path traversal keeps its own existing bounds. Malformed state or real
path/getter/resolver errors propagate as VM statuses rather than being fabricated
as an eligibility result. Allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44).

The candidate, receiver and argument graphs are rooted throughout nested script.
Trusted Selection entries permit collection in script getters; enclosing ordinary
native barriers remain in force. A getter can change focus, mutate properties,
remove clips or throw. Those side effects remain committed even if outer selection
fails. A nested successful selection is overwritten if the outer attempt later
succeeds; there is no rollback or focus-change event queue. No notification is
emitted merely by setFocus or by clearing it. Selected clips receive subsequent
keyboard transitions through [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L79), before independent
Key listeners, according to the shared host policy.

## getFocus

`Selection.getFocus()` ignores arguments and returns null if nothing is selected,
or a counted native dot target path such as `_level0.panel`. A removed clip or
physical ancestor lazily clears the shared selection and yields null. The path
comes from native scene state; replaceable clip.toString/_target methods do not
run. Existing native naming/level/Unicode rules and traversal bounds apply.

The returned string is borrowed context-owned storage: consume it before another
allocating call, or retain it with the ordinary value/string rooting APIs. No
object ownership is transferred. Query errors, including native validation/path
allocation failure, propagate through normal VM status/exception handling. A
successful detachment check can clear stale focus before later work fails.

## Limits and interoperability

Selection and host C focus APIs share one strong context root. The host setter
bypasses script eligibility; getFocus still describes that host-selected clip.
State persists until explicit clearing/replacement, lazy detachment checking or
context destruction, including across movie load and namespace replacement.
It is unsynchronized, host-local and not serialized; namespace methods have no
portable serialization callback IDs and graph transfer returns [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46).

TextField/caret/range methods, Button objects, Selection listeners, onSetFocus /
onKillFocus callbacks, focus rectangles, automatic mouse/Tab navigation, IME,
security origins, SWF ClipActions and original-player edge/order conformance
remain open. The getFocus dot-path format and callable-handler eligibility are
the documented subset policy; exhaustive reference-player acceptance is pending.
The original rules are described by Adobe's AS2 reference
([Selection mirror](https://open-flash.github.io/mirrors/as2-language-reference/Selection.html),
[MovieClip mirror](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html)).

## Examples and validation

- [Executable C89 tutorial](../examples/embedding/selection/README.md): default
  rejection, focusEnabled, getFocus and clearing with explicit lifetime management.
- [Interactive SWF](../examples/player/animation_audio/README.md): selects its
  root by `_root` path, separately counts focused clip and Key listener events,
  and increases the blue box height on root-focused releases.
- `tests/selection.cpp`: Unicode paths, eligibility, invalid types, native getters,
  real failure propagation, removal during eligibility and lazy ancestor clearing.
- `tests/selection_install_allocation.cpp`: fresh-context installation allocation
  sweep and setFocus allocation failures preserving the previous focus.
- `tests/jit_selection.cpp`: SWF6–8 interpreter, explicit JIT and automatic JIT,
  real script property getters that collect, versioned global conflicts and callbacks.
- `frontend/tests/keyboard_movie.cpp`: actual shipped SWF, three loops, complete
  taps before a tick, matching independent counters and one listener in both modes.

Host/runtime fixtures and cross-builds are distinct from physical keyboard/device
acceptance. See [current roadmap status](ROADMAP_STATUS.md).

Implementation / Реализация: [`fvm_selection_set_focus`](../src/standard/selection.cpp#L53), [`fvm_selection_get_focus`](../src/standard/selection.cpp#L101)
in [src/standard/selection.cpp](../src/standard/selection.cpp).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:79](../include/flashvm/keyboard.h#L79) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:66](../src/standard/movieclip_constants.h#L66) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_selection_get_focus` | function | [src/standard/selection.cpp:101](../src/standard/selection.cpp#L101) |
| `FVM_SELECTION_MIN_SWF_VERSION` | macro / define | [include/flashvm/keyboard.h:5](../include/flashvm/keyboard.h#L5) |
| `fvm_selection_set_focus` | function | [src/standard/selection.cpp:53](../src/standard/selection.cpp#L53) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:18](../include/flashvm/standard.h#L18) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:83](../include/flashvm/keyboard.h#L83) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
