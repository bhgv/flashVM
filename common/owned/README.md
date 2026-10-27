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
