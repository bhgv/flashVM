# Projection inspector acceptance example

[Contract and limitations](../../PROJECTIONS.md) · [Русский](README.ru.md)

`main.b` is a fully commented executable regression. It loads the pure Limbo
`Ideprojection` and `Ideinspect` modules. Two spawned target owners hold separate
state; a third process owns the watch rows. The caller sends typed requests and
consumes one typed response for every request. There are no native target pointers,
source-expression evaluation or deployed Python dependencies.

The test starts at integer 7, rejects an unapproved add call, explicitly adds 5,
rejects a stale write, transfers `Привет 😀`, reads during cooperative pause,
rejects paused invocation and resumes a checked write. It releases both owners.
It then checks out-of-order refresh admission, a real owner's response, renderer
copy isolation, duplicate rejection and selection replacement. Explicit `fail:`
exceptions identify violated contracts; success prints a PASS line.

Build with the native runtime's matching compiler and headers:

```sh
cmake --build build/runtime
ctest --test-dir build/runtime -R projection_inspector --output-on-failure
```

The runtime must already be configured following its [guide](../../../../disVM/runtime/README.md).
The build produces `ideprojection.dis`, `ideinspect.dis`, `projection-check.dis`
and debug symbols beside the matching runner. Run in that directory:

```sh
./disvm --stats --no-jit projection-check.dis
./disvm --stats --jit projection-check.dis
```

The CTest wrapper checks actual native dispatch counters, not just printed text.
This is a local Dis-process boundary. It does not claim live network RPC or a
finished graphical inspector. Pending target operations must run in workers,
not on the view-model owner; callers must never abandon an unconsumed rendezvous.


## Explicit values and bound receivers

The extended example includes `idevalue.m` before the projection/model interfaces.
All `.m` clients must be rebuilt together. `ReadAs(number, Real)` returns a copied
binary64; mutating it cannot change the target. `ToTable` copies stored fields
but its self entry is an Object reference. `InvokeRef` calls the returned bound
method at its receiver and rejects a different receiver or incarnation.

`Convert` demonstrates an exact Big constant (9223372036854775807) sent as data
through a real channel. `WriteValue` detaches the caller's wrapper before storing
an exact matching typed scalar. The view model receives an actual owner's typed
table and protects itself from changes to both sender and renderer copies.
See [conversion rules and type reflection](../../VALUES.md).
