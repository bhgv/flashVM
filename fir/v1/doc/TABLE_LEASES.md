# Retained ownership of table assignment destinations

[Русский](TABLE_LEASES.ru.md) · [Commented source](../examples/tables/leases.b) ·
[Compound arithmetic](COMPOUND_UPDATES.md) ·
[Concurrent native tutorial](../../../disVM/examples/table-leases/README.md)

Our Coco/R Limbo big/table profile now emits a retained ownership scope for every
indexed, dotted or snapshot-key table assignment, including compound updates and
resized slice writes. The receiver is evaluated once. The generated sequence is:

```fir
    table.lock %receiver
    # Evaluate key/bounds and capture the absolute destination here.
    # For compound updates, read the old value before evaluating the RHS.
    # Evaluate the RHS, including reads through aliases of this receiver.
    table.store %location, %value
    table.unlock %receiver
```

A slice uses `table.splice` instead of `table.store`. The lock precedes key/bound
computation, so relative indexes cannot silently resolve against a different
prefix before publication. The receiver remains the same SSA owner throughout.
Private constructors need no explicit retained scope before publication. Local
big rebindings and immutable string replacement emit no table ownership effects.

`table.lock` and `table.unlock` are void unary FIR effects accepting only
`table.big`. They produce no SSA result. Lock reads/writes memory and may throw
or suspend; unlock reads/writes memory and may throw. Dead-code elimination must
preserve both even when no computed result escapes. Debug JSON events
`table-lock` and `table-unlock` carry the destination variable occurrence. These
are source-to-FIR spans, not a claim of complete source-to-Dis SBL generation.

The NC64 backend uses existing Dis MFRAME/MCALL instructions and signature-checked
`$Bigtable.acquire/release` imports. No Dis opcode or container-format change is
introduced. An owning Dis process may reenter its table. A conflicting mutable
payload call parks its process before native-frame publication; the pending
frame and retained table remain GC roots. Metadata diagnostics continue to run.
Interpreter and leaf JIT use this same native boundary.

On any application exception, the runtime aborts **all** retained table scopes of
the throwing process before searching for a handler. Normal process retirement
also releases them. Already completed mutations are not rolled back: this is
exclusive ownership, not a multi-object transaction. Multiple retained owners
must follow the runtime's stable guard order; reverse acquisition or a reverse
conflicting wait raises rather than deadlocking. That internal order must not
be serialized as a remote identity. A channel protocol can still deadlock if an
owner waits for a peer that requires its table.

The bounded source profile currently has no arbitrary function calls, spawn or
channel receive in these RHS expressions. The explicit native Limbo tutorial
checks actual contention across channel waits separately. Automatic source tests
check ordering, alias/self-splice behavior, optimizer preservation, invalid types,
source spans, GC roots, both VM modes and quantum 1/default scheduling. The
optional table reference runner is sequential; its scope effects are no-ops
after FIR type verification and do not constitute concurrency validation.

This extension stays in FlashVM. General arrays, mutable string slots, proxy/any
assignment scopes, distributed lease cancellation/timeouts and automatic recovery
are separate pending work. Explicit AS conversions retain their AS syntax;
Limbo still uses typed `big`, `int` and `string` casts.
