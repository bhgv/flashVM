# Generic AsBroadcaster events

Available since v0.615. This is an explicit, bounded SWF6–8/host subset of the
[Adobe ActionScript 2 AsBroadcaster reference](https://open-flash.github.io/mirrors/as2-language-reference/AsBroadcaster.html).
It supplies application-defined event objects independently of SDL, Clutter3d,
GTK, Qt or Linux devices. Existing Key, Mouse and Selection registrations keep
their own documented contracts. [Русский](AS_BROADCASTER.ru.md).

## Installation and ownership

Include `flashvm/broadcaster.h`. On the VM owner thread, after
[`fvm_standard_install`](../include/flashvm/standard.h#L19), call `fvm_standard_broadcaster_install(context)` to publish
the ordinary namespace object `AsBroadcaster`, with native `initialize(object)`.
The namespace is opt-in in the embedding ABI. Since v0.616 the shipped player
explicitly installs it for SWF6–8 before scene execution; standard installation
alone does not enable it. No constructor is supplied. Installation returns [`FVM_OK`](../include/flashvm/flashvm.h#L42) for an
intact prior installation; conflicting global properties, methods, accessors or
function registries return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45) without replacement.

`fvm_broadcaster_initialize(context, object)` initializes a non-null, same-context
ordinary application object directly; namespace installation is unnecessary for
this C entry. It adds three non-enumerable, non-deletable native methods and the
read-only, non-enumerable, non-deletable `_listeners` accessor. The accessor retains
a live native Array. Reinitializing an intact object replaces the Array with a
new empty one and preserves unrelated own properties, prototype and resource
state. An existing conflicting own reserved slot causes [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45).
Inherited slots can be shadowed. Prefer ordinary user objects.

Both APIs require valid installed Object/Array bindings. Their validation can
execute user getters on those globals; such errors propagate, and getter side
effects remain committed. The receiver is rooted before validation. Subsequent
preparation allocates without callbacks, then publishes by non-allocating swaps.
[`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) during preparation preserves published receiver/global slots;
temporary unreachable staging objects are reclaimed by later collection.

Both APIs reject null/unusable lifecycle state with [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45), and
loaded SWF versions below 6 with [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46), even when installation
preceded loading. Allocation errors propagate as [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44); other native
exceptions are contained as [`FVM_ERROR`](../include/flashvm/flashvm.h#L43). Call outside collection, release and
teardown. No cross-thread access, automatic event pumping or ownership transfer
is provided. Keep a native object pinned or otherwise reachable while constructing
or retaining it. Initialization does not pin its receiver permanently.

## Script methods and delivery

| Member | Parameters and result |
| --- | --- |
| `AsBroadcaster.initialize(object)` | Same-context Object; install/reset its broadcaster state. Missing/non-object argument does nothing. Returns undefined. Native initialization errors propagate. |
| `addListener(listener)` | Object identity; retain once in the live Array. Duplicate registration succeeds without moving its order. Missing/primitive input does nothing. Returns true on success. |
| `removeListener(listener)` | Remove the first matching Object and compact ordinary stored indices. Returns true if found, false otherwise; missing/primitive input returns false. |
| `broadcastMessage(name, ...payload)` | Coerce name with AVM1 string rules, then take a strongly rooted recipient snapshot. Invoke each callable matching member with listener as this and payload only; return undefined. Missing argument does nothing. |
| `_listeners` | Read-only binding to the same live native Array until reset. Ordinary writes to the binding are ignored. This is internal registration storage, not a detached inspection copy. |

Handlers use ordinary version-aware inherited/accessor/script/native member lookup.
Counted names preserve embedded NUL and Unicode; no global function fallback is
used. Missing or non-callable handlers are skipped. Errors from an actual getter
or handler stop delivery and propagate unchanged. Earlier handler side effects
remain. A handler return value does not cancel delivery. Registration changes
inside a handler affect the next broadcast: a removed snapshot recipient still
receives the current event; newly added recipients wait. Reinitialization likewise
does not rewrite an active snapshot. Recursive calls share execution-work budget.
The native traversal charges list scanning and recipient dispatch to that budget.

[`FVM_BROADCASTER_MAX_LISTENERS`](../include/flashvm/broadcaster.h#L6) is 4096. A larger/invalid native list length or
an insertion at the bound returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43); malformed non-Array storage returns
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Registration and removal stage changed index maps before
publication. Dispatch allocation failure before the first handler runs delivers
nothing; later handler allocation failure preserves already completed calls.

## Graphs and limits

The native callbacks use stable graph IDs. Serialization preserves the live Array,
shared edges, cycles and accessor capture roots; decoding restores trusted GC entry
for the native methods. The sender may be destroyed before decoding. Native user
handlers still require the ordinary stable callback registration in each context;
arbitrary userdata is not serialized. Existing graph limits apply. Older receivers
without these callback IDs reject the graph as unsupported; the graph format and
public ABI version have not changed. No complete channel/worker/platform acceptance
is claimed by the direct graph fixture.

This profile deliberately has bounded object listeners and snapshot semantics.
Historical primitive boxing, duplicate reorder rules, live mutation iteration,
constructor/static-helper reflection and original-player comparison remain open.
Treat direct descriptor, accessor, hole, shadowed-length or index-attribute edits
to `_listeners` as outside the accepted profile; broadcaster operations use native
stored length and ordinary stored indices. The live Array's standard independent
API does not certify every such interaction. Keep registration changes in the
three methods above.

## Tutorial and verification

[Annotated C89 and AS example](../examples/embedding/broadcaster/README.md).
Tests: `broadcaster` (arguments, counted names, mutation, trusted GC, real errors,
conflicts, reset, version guards), `broadcaster_allocation` (allocation sweep over
installation, first initialization, reset, add/remove and broadcast preparation), `broadcaster_graph`
(cyclic clone after sender destruction), `jit_broadcaster` (loaded SWF6–8,
interpreter/manual/automatic JIT and nested script GC, actual native entry checks
when JIT is available). These are Linux acceptance fixtures, not evidence for
physical devices, other operating systems or complete Flash compatibility.

## Transport and player v0.616

The C89 tutorial now clones a broadcaster between two owner-thread contexts using
[`fvm_channel_send_value`](../include/flashvm/transfer.h#L22) and [`fvm_channel_receive_value`](../include/flashvm/transfer.h#L25), with explicit native callback
registration in each heap. Payload 42 reaches the original, 84 reaches the copied
listener after destination collection. See the tutorial's ownership/API table.

The broadcaster_transport fixture covers SWF6–8 across memory queues, durable
files and local TCP broker channels. It sends a cyclic graph containing a script
handler and shared cached Function wrapper, resets/destroys the sender, then
collects and invokes the imported graph in interpreter, manual and automatic JIT
modes when native JIT is available. The handler calls gc.collect; recipient counters,
receiver identity, payload, wrapper identity, removal/reset and actual native
entries are checked. Independent writer/reader processes additionally verify file
records across separate address spaces. These tests use cloned function bodies,
not live remote function references or arbitrary native userdata.

The shipped common player explicitly installs AsBroadcaster for SWF6–8. A separate
CLI fixture generates self-checking movies for those versions, plus a SWF5 startup
guard, and checks interpreter/JIT execution with SDL dummy video and muted audio.
A wrong-payload negative control must fail playback. This is host integration and
software-protocol evidence, not physical display/audio or foreign-OS acceptance.
The namespace is independent of the display API; other hosts choose installation
through the same C89 entry. No changes to Clutter3d or inferno-os are required.

Worker races, long-running/remote-network failure acceptance and full historical
Flash broadcaster semantics remain open; bounded loopback/file cases do not close
roadmap stages 6–9.

## Registration work limits (v0.617)

Registration shares the context's `FVM_Config.instruction_limit` with nested
AVM1 execution and broadcasts. Each examined logical index, copied stored index
and new registration is charged. Duplicate and missing-listener searches also
consume work. Zero disables the numeric limit; worker cancellation is still polled.
These are abstract work units, not a wall-clock deadline or byte allocation limit.

Add/remove prepare a new index map before publication. Exhaustion or cancellation
returns [`FVM_ERROR`](../include/flashvm/flashvm.h#L43) with the live Array identity, length, order and entries intact.
No partial registration is published, including failure midway through copying.
Nested calls cannot obtain a fresh budget. A later independent host call starts
its own budget. Dispatch snapshot semantics and existing handler side effects
remain unchanged. Graph decoding is not charged by this registration mechanism.

`broadcaster_budget` verifies SWF6–8 direct calls, extracted native functions,
interpreter/manual/automatic JIT, failed search/copy/compaction, retry after GC,
and shared nested budgets. This is bounded Linux execution evidence; it does not
close roadmap stages or certify other platforms.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_broadcaster_initialize` | function | [include/flashvm/broadcaster.h:19](../include/flashvm/broadcaster.h#L19) |
| `FVM_BROADCASTER_MAX_LISTENERS` | macro / define | [include/flashvm/broadcaster.h:6](../include/flashvm/broadcaster.h#L6) |
| `fvm_channel_receive_value` | function | [include/flashvm/transfer.h:25](../include/flashvm/transfer.h#L25) |
| `fvm_channel_send_value` | function | [include/flashvm/transfer.h:22](../include/flashvm/transfer.h#L22) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_broadcaster_install` | function | [include/flashvm/broadcaster.h:13](../include/flashvm/broadcaster.h#L13) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:19](../include/flashvm/standard.h#L19) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
