# Selection MovieClip subset

v0.592 adds explicit Selection.getFocus/setFocus for native MovieClips in host
contexts and SWF6–8. [Русский контракт](SELECTION.ru.md). It shares the
[host keyboard focus root](KEYBOARD_FOCUS.md) and existing display-path resolver.
This is a bounded implementation, not complete Flash Selection compatibility.

## Installation

`fvm_standard_selection_install(context)` requires a borrowed non-null live
context with standard objects already installed by [`fvm_standard_install`](../include/flashvm/standard.h#L20).
Use the VM-owning thread, outside context teardown or GC collection/release.
It synchronously prepares a namespace containing getFocus, setFocus, addListener, removeListener and a read-only
_listeners snapshot accessor,
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
later loads SWF1–5, all namespace entries return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), preserving focus;
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
getter may remove the candidate. [`FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS`](../src/standard/movieclip_constants.h#L69) bounds that
native walk; path traversal keeps its own existing bounds. Malformed state or real
path/getter/resolver errors propagate as VM statuses rather than being fabricated
as an eligibility result. Allocation failure returns [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44).

The candidate, receiver and argument graphs are rooted throughout nested script.
Trusted Selection entries permit collection in script getters; enclosing ordinary
native barriers remain in force. A getter can change focus, mutate properties,
remove clips or throw. Those side effects remain committed even if outer selection
fails. A nested successful selection is overwritten if the outer attempt later
succeeds; there is no rollback or focus-change event queue. Successful transitions use the
synchronous notifications described below. Selected clips receive subsequent
keyboard transitions through [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L90), before independent
Key listeners, according to the shared host policy.

## Focus notifications

Since v0.593, an accepted identity change first publishes the new focus, then
calls `oldClip.onKillFocus(newClip)` and `newClip.onSetFocus(oldClip)`, omitting
missing endpoints and representing no focus by null. Selecting the same identity,
rejected eligibility, host C focus setters and lazy detachment clearing are silent.
Methods use normal versioned/inherited/accessor lookup, preserving the actual
receiver. Missing or noncallable handlers are ignored; global functions with the
same name are never used as fallbacks. Handler return values are ignored.

Both endpoints are rooted before publication through the complete synchronous
transition. Script handlers/getters may collect; ordinary native callback barriers
remain active. A callback may change focus recursively: that nested change remains
current, while the outer transition finishes notifying its original endpoint
snapshot. Handler lookup occurs at delivery time, so a first handler may replace
the second handler. No notification queue or rollback is provided. Existing VM
script recursion limits apply to recursive script callbacks.

A real getter/handler error, including allocation failure, stops further delivery
and propagates its original status. Already published focus and handler side
effects remain committed. Thus an out-of-memory failure before publication keeps
the previous selection; one during notification keeps the published selection
unless a callback changed it. Even [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) from a real callback is
an error, not the Boolean false used for an ineligible candidate. Removing a
snapshot endpoint during a callback does not cancel its pending notification;
subsequent getFocus applies the ordinary lazy detachment check. These are explicit
subset ordering rules; original-player edge-case acceptance remains open.

## Selection listeners

Since v0.594, `Selection.addListener(object)` returns undefined and retains one
same-context object identity; duplicate additions leave registration order intact.
No arguments, null, undefined or primitive values are ignored. Callback argument
validation rejects foreign-context values with [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before entry.
Registration may return [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) without adding an identity; reaching
[FVM_SELECTION_MAX_LISTENERS](../src/standard/selection_listeners.hpp#L6) (4096) returns [FVM_ERROR](../include/flashvm/flashvm.h#L43). No callbacks or devices run
during registration. `Selection.removeListener(object)` returns true only for a
removed identity, false for absent/invalid script inputs; removal itself allocates
no memory, though the enclosing method call may allocate its argument snapshot.

Before a real focus change publishes anything, it copies and roots the current
registrations. Preparation OOM preserves the previous focus. After successful
per-clip notifications, each snapshot receiver receives
`listener.onSetFocus(oldClip, newClip)` in registration order, using object or null
endpoints. The receiver is the listener, and handler lookup occurs once at delivery
time. Missing/noncallable handlers are ignored without global fallback; real
getter/handler errors stop delivery and propagate with the focus already committed.
Nested changes run synchronously with independent snapshots and retain their result.
Selecting the same clip, rejecting eligibility, silent C host setters and lazy
removal checks do not notify observers.

Adding/removing registrations during a callback affects later transitions only.
Removing a later listener and collecting cannot cancel its pending notification;
the active snapshot roots it. Modifying a pending listener's method affects its
later delivery. Strong registrations are context-owned, independent of Key and
public Selection namespace replacement; they end on explicit removal or context
destruction. They are unsynchronized and not serialized. A transported ordinary
listener object is not automatically registered in the destination context.

`Selection._listeners` returns a fresh native Array with the current identities,
without executing a replaceable Array constructor. The accessor is non-enumerable,
non-deletable and has no setter. Editing its Array does not change registrations;
root a retained result before any collecting call. Allocation errors propagate
without registry changes. This is an inspection snapshot, not original Flash's
mutable broadcaster-array identity contract. Namespace reinstallation validates
all four method identities plus the unshadowed snapshot accessor.

The basic signatures follow the Adobe AS2 Selection reference linked below.
Snapshot mutation, nested notification ordering, limits and namespace replacement
are explicit subset policies; original-player acceptance is still required.

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

TextField/caret/range methods, Button objects, focus rectangles, automatic mouse/Tab navigation, IME,
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
- `tests/selection_listeners.cpp`: identity, detached inspection snapshots, mutation
  with GC, committed failures, namespace replacement and independent contexts.
- `tests/selection_listeners_allocation.cpp`: registration/snapshot/preparation/
  post-publication allocation sweeps and root restoration.
- `tests/selection_notifications.cpp`: arguments/order, identity no-op, committed
  errors, recursive selection and collection after both persistent roots are removed.
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

Implementation / Реализация: [`fvm_selection_set_focus`](../src/standard/selection.cpp#L128), [`fvm_selection_get_focus`](../src/standard/selection.cpp#L178)
in [src/standard/selection.cpp](../src/standard/selection.cpp).


## v0.618 shared event budget / Общий бюджет событий

Registration, inspection and delivery share nested AVM1 work limits. Preparation
failure preserves valid state; later failure keeps committed input and callbacks.
[EN contract](INPUT_EVENT_BUDGET.md) / [RU](INPUT_EVENT_BUDGET.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:90](../include/flashvm/keyboard.h#L90) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_MOVIECLIP_REMOVE_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:69](../src/standard/movieclip_constants.h#L69) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_selection_get_focus` | function | [src/standard/selection.cpp:178](../src/standard/selection.cpp#L178) |
| `FVM_SELECTION_MAX_LISTENERS` | macro / define | [src/standard/selection_listeners.hpp:6](../src/standard/selection_listeners.hpp#L6) |
| `FVM_SELECTION_MIN_SWF_VERSION` | macro / define | [include/flashvm/keyboard.h:5](../include/flashvm/keyboard.h#L5) |
| `fvm_selection_set_focus` | function | [src/standard/selection.cpp:128](../src/standard/selection.cpp#L128) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:101](../include/flashvm/keyboard.h#L101) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
