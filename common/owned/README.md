# Operation-owned payload guard

[Русский](README.ru.md) · [VM integration contract](../../disVM/OWNED_OPERATIONS.md)

This C99/POSIX library implements an exclusive recursive guard keyed by a nonzero
operation token, rather than by native thread identity. Contenders sleep on a
condition variable; the metadata mutex is never held while accessing payloads.
It is a host building block, **not yet wired into Dis arrays, strings or tables**.
Windows requires a native backend before this library can be used there.

The owner allocates stable guard storage alongside a payload/storage owner.
Array views sharing storage must share this same guard. Never copy an initialized
guard. Token allocation, lifetime, GC roots, and multi-object ordering belong to
the integrating runtime; remote callers must not supply trusted owner tokens.
A token identifies one sequential continuation: simultaneous native execution
with the same token is forbidden, even though sequential recursion is allowed.

Disable pthread cancellation before entry, keep it disabled through the entire
operation, balance every successful enter with leave, then restore the prior
cancellation state. Entry rejects an enabled cancellation state with EINVAL.
Use cooperative cancellation: network timeout or channel loss cannot release a
guard still held by executing work. Native asynchronous cancellation is unsupported.

```c
/* guard was initialized before publishing its associated payload. */
int previous;
int error = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, &previous);
if (!error) {
    error = fvm_owned_enter(&guard, operation_token);
    if (!error) {
        /* Validate bounds and obtain snapshots only after this entry.
         * Nested helpers use this same token. All payload readers must also
         * enter: unguarded reads racing a mutation are not safe. */
        mutate_payload();
        /* VM exceptions must route through cleanup before escaping. */
        error = fvm_owned_leave(&guard, operation_token);
    }
    pthread_setcancelstate(previous, NULL);
}
```

`inspect` takes only the short metadata lock and returns owner/depth/waiters;
it does not touch application data. It can serve broker/debugger diagnostics
while a payload operation is paused. This does not permit running a second
application continuation with the paused owner's token.

`destroy` requires external quiescence: stop publishing the guard, join/drain all
users, and then destroy. Its busy check is not a substitute for lifetime ownership.
Locks have no fairness or bounded-wait guarantee. Never block the single VM owner
thread on work which itself needs that thread; runtime integration needs a
scheduler-aware boundary before waiting on externally executing operations.

Build with `cmake -S common/owned -B build/owned`, then
`cmake --build build/owned` and `ctest --test-dir build/owned`.
The test parks four native contenders behind a twice-entered owner, inspects the
live guard, checks wrong-token release and busy destruction, then performs 40,000
protected mutations with nested entries. CTest bounds unexpected deadlock to 20s.

## Guarded scalar table owner

`table_owner.h` adds `FvmTableOwner`: an Fht table and its operation guard share
one explicit native lifetime. `fvm_table_owner_apply` protects the complete
callback transaction, including reads, length checks and writes. Nested helpers
reenter with the same token. Callback application errors are separate from
native guard errors; normal error returns release ownership. Callbacks must not
raise VM exceptions or longjmp past cleanup. This host API is not yet used by
DisVM's native Bigtable module, nor by array/string storage.

`fht_splice(table, first, end, replacement)` replaces a dense-prefix range by the
replacement table's complete prefix. It supports self insertion, growth,
contraction and deletion. Build a source slice first to insert just part of a
replacement. Text keys retain insertion order; sparse numeric keys remain at
absolute positions. Growth colliding with an existing sparse numeric key fails
with FHT_INVALID, leaving the table unchanged. All storage is prepared before
commit; allocation failures also preserve original contents. Serialize both
owners for a cross-table splice, in stable runtime-defined order. A self splice
needs only its own recursive guard.

The additional native contention test runs 40,000 read-modify-write transactions
on a real table through nested owner callbacks. Core table tests additionally
inject allocation failures into self-splice and check contents and live blocks.

## Executable tutorial

`example_splice.c` is a fully commented owner-lifetime example. Build the default
target, then run `build/owned/owned_splice_example`. It populates `[1, 9, 4]`
under one operation, inserts the original prefix into itself under another, and
checks `[1, 1, 9, 4, 9, 4]`. Both callbacks release the guard on ordinary error
returns. Native cancellation is restored only after retiring the table. The
example is also run by CTest; it demonstrates the C host API, not Limbo syntax.
