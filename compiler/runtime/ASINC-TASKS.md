# Executable cooperative task lifetime owner

[Русский](ASINC-TASKS.ru.md) · [Language contract](../ASINC.md) ·
[Directional capture publication](ASINC-VIEWS.md)

`asinctasks.dis` is an ordinary portable Limbo module. A scheduled sole owner
serializes typed requests while separate Limbo processes claim and execute work.
It runs on the current interpreter and JIT without waiting for a Flash timeline
frame. It imports no Draw/Tk, native callback or platform-specific worker API.

This is executable groundwork for `asinc`, **not AS closure lowering**. Captures,
results and error payloads are immutable strings. Mutable object graphs, actual
AS lexical environments, physical AS worker threads, compiler/FIR task lowering,
forced cancellation and new Dis64/native ABI remain outside this module.

## Protocol

Call `start(positiveIncarnation)`; invalid generation returns nil. The caller must
choose a new incarnation when replacing an owner while old messages can exist.
Each request carries that exact generation and a consumed reply channel. Old and
future generations return Stale without changing state. Incarnations and worker
identities prevent accidental stale delivery; they are not security credentials.

| Operation | Accepted behavior |
| --- | --- |
| Submit | Copy an input string of at most 65536 characters; allocate monotonic task ID |
| Claim | Positive worker takes first queued task, input and a claim lease; empty returns immediately |
| Complete / Fail | Exact worker+lease supplies result/error; terminal state entered once |
| Cancel queued | Enter Cancelled and release owner input immediately |
| Cancel running | Enter CancelRequested; retain owner input until worker acknowledges |
| Complete / Fail after cancellation | Acknowledge worker termination, discard late value, enter Cancelled |
| AcknowledgeCancel | Explicit worker acknowledgement after cancellation request |
| TakeResult | Completed/Failed result transfers once; repeated take returns Taken |
| Snapshot | Copy task records for inspection; modifying records/strings cannot mutate owner |
| Shutdown | Busy while Running/CancelRequested; otherwise discard remaining owner roots and exit |

The owner's receive order decides completion versus cancellation. Completion first
remains Completed/Failed; later Cancel returns Terminal. Cancellation first must
wait for a matching finish or acknowledgement. A lost/noncooperative worker can
therefore keep shutdown Busy indefinitely. The caller must arrange worker error
handling and acknowledgement; this service does not discover dead processes.
Never send to an acknowledged stopped owner: an unbuffered send has no consumer.

There are 32 lifetime slots. Terminal slots are not recycled, preventing an old
finish from naming a new task. Claim leases equal their unique task IDs in this
single-claim prototype; retries and requeue do not exist. At capacity Submit
returns Full. Start a new owner with a new incarnation only after quiescence.

## Ownership and GC boundary

The owner retains input while Queued, Running or CancelRequested. Claim returns
an independently rooted scalar copy to the worker. Completing/acknowledging clears
the owner's input; a worker's local reference lives until that process releases
it. Complete/Fail retain the result independently until TakeResult, which roots
it in the reply before clearing the owner slot. Empty strings use explicit held
flags, so an empty valid result is distinguishable from no result.

Snapshots copy the record array; strings use Limbo value/COW semantics. A snapshot
can itself retain copied values after the owner releases them. `inputHeld` and
`resultHeld` report owner-slot lifetime, not the entire VM heap's live-root count.
Consumed request/reply payload fields are explicitly cleared before the next receive,
so idle service stack slots do not retain previous transport values.
Allocation exceptions propagate; recoverable OOM rollback is not implemented.
These tests do not replace Inferno's forced-collector native environment/result
gates. Directional capture publication remains the separate `asincviews` protocol;
no implicit frame hook or connection between the services has been introduced.

## Build, run and inspect

```sh
cmake -S disVM/runtime -B build/tasks -DDVM_RUNTIME_TESTS=ON
cmake --build build/tasks
ctest --test-dir build/tasks -R asinctasks --output-on-failure
(cd build/tasks && ./disvm --no-jit tasks-demo.dis)
(cd build/tasks && ./disvm --jit tasks-demo.dis)
```

Use the runtime's normal module search path containing its built `.dis` modules
when invoking binaries directly. The installed package includes `tasks-demo.dis`,
`asinctasks.dis`, header, source and both language guides. The regular installed
runner can also run `tasks-demo.dis` in interpreter/JIT mode. No Python runtime
is required by the bytecode application.

[Commented example](../../disVM/examples/asinctasks/main.b) loads the service,
submits an input, spawns a real Limbo worker, takes its result and shuts down.
[Acceptance source](../../ide/dis/tests/asinctasks_test.b) forces both race orders
with channels rather than sleeps; it checks stale generations, wrong worker/lease,
snapshot isolation including a string-character write, duplicate completion,
queued/explicit cancellation, failures, capacity and shutdown while active.
