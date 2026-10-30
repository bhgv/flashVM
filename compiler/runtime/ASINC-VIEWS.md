# Executable scalar capture publication prototype

[Русский](ASINC-VIEWS.ru.md) · [Language contract](../ASINC.md)

`asincviews.dis` is an ordinary Limbo module running on our DisVM interpreter/JIT.
It implements the agreed direction of capture exchange for **immutable string
scalars**. It does not execute ActionScript `asinc`, install onChangeVarAsinc,
run an OS thread, hook actual timeline frames or implement mutable object graphs.
Its boundaries are explicit typed requests, suitable for testing the contract
before connecting compiler/runtime hooks. It imports no Draw/Tk or native service.

## Publication

Owner0 and up to8 stable tasks see16 bindings. BeforeFrame publishes dirty task
values into the owner; AfterFrame publishes owner values into clean task views.
Only local writes create versions/dirty bits. Received values retain the original
writer/version and do not echo. A task write during frame computation survives
AfterFrame when the frame changed another binding. Equal scalar assignments do
not allocate a new cell or change its writer version.

Private immutable cells and strings share storage between views. A changed Write
allocates a replacement cell: scalar copy-on-write. There is no pointer crossing
between independent VM heaps. Nested graph/path COW and alias-preserving mutable
objects require a separate implementation and are not simulated by strings.

Multiple dirty task writers to one binding, or concurrent frame/task writes to
one binding, return Conflict before any publication. This is explicit refusal,
not an agreed last-writer-wins policy. Discard(task,binding) explicitly drops that
task's local write and takes the current owner version, allowing a caller to
resolve a conflict. No implicit discard occurs at a boundary.

## Observations

Subscribe(target,binding,"on"/"off") controls future remote-change notifications.
BeforeFrame can notify owner0; AfterFrame can notify clean task subscribers.
An event contains subscriber/binding, original writer, write version, old/new
scalar values. Equal values, own writes and echoed versions create no event.
Drain(target) removes only that subscriber's events in publication order; no
callback executes under the owner. Turning a subscription off retains events
already queued. An actual callback dispatcher and compiler intrinsic are pending.

Snapshot returns detached flat arrays indexed `task*Bindings+binding`: values,
versions, original sources, dirty bits, plus task lifecycle. Modifying returned
arrays cannot mutate state. Inspection shows current private views, including
local dirty writes at this serialized service boundary. It is a coherent snapshot,
not an unsynchronized live map of another physical thread. A future public API
must distinguish local/private from last-published views explicitly.

The256-event global queue is preflighted for the **whole** phase transfer. Full
leaves phase, all values and dirty bits unchanged; drain and retry are safe.
This prototype returns Full rather than dropping events or blocking on callbacks.
Only publication changes are events; intermediate assignments are not retained.
Logical Invalid/Stale/Conflict/Full refusals are atomic. VM allocation failure is
not translated into a recoverable status by this module.

## Lifetime and protocol

Spawn accepts never-used IDs1..8 only between frames and captures owner cells.
Retire clears its cells/dirty/subscriptions and queued subscriber events. IDs are
never reused in one service lifetime, avoiding confusion with reused OS tids.
Other subscribers can retain older events naming the retired original writer.
Shutdown is acknowledged before the owner exits and drops its private roots.

All mutations require an exact revision lease. Snapshot ignores it. Invalid
commands do not advance revision; accepted commands (including equal writes and
empty drains) do. Phase must alternate BeforeFrame/AfterFrame; owner writes are
admitted only during computation. String writes are limited to4096 characters.
The consumer must receive each acknowledgement; this owner is a scheduled Dis
process and does not claim physical thread parallelism.

## Build, run and inspect

[Short commented example](../../disVM/examples/asincviews/main.b) demonstrates
task-private writes, both boundaries, event inspection and no-echo. The installed
SDK ships it as examples/asincviews/main.b and capture-demo.dis.

From the repository, build the editor runtime with its existing explicit options:

```sh
cmake --build build/workspace/disvm-editor --target disvm_native_modules disvm_native_asincviews
ctest --test-dir build/workspace/disvm-editor -R application_asincviews --output-on-failure
```

Installed SDK (run from its root; the shipped compiler produces native-cell Dis):

```sh
./limbo-native -g -I include/limbo -o asincviews.dis source/asincviews.b
./limbo-native -g -I include/limbo -o asincviews-check.dis examples/asincviews/acceptance.b
./disvm --jit --stats capture-demo.dis
./disvm --jit --stats asincviews-check.dis
./disvm --no-jit --stats asincviews-check.dis
```

The SDK ships its matched native-cell compiler as `limbo-native`. The acceptance source documents each scenario: direction,
no echo, task writes during frames, conflict rollback, detached snapshots, owner
notifications, full queue/drain/retry, stale requests and task retirement.
This executable prototype is a separate checkpoint from syntax/frontend tests
and eventual executable SWF/Dis asinc lowering.

[Verification / Проверки](../../docs/releases/v0.1700.md).
