# styxrpc_managed_invoke

[Русский](managed_invoke.ru.md) · [Declaration and target](../include/styxrpc/managed_client.h) · [Implementation](../src/managed_client.c)

Include `styxrpc/managed_client.h` and link the hosted target `StyxRpc::launch`.

```c
StyxRpcOutcome styxrpc_managed_invoke(const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity,
    StyxRpcManagedReply *result);
```

Provide a mounted session, relative module path, export name, compiled
signature and exact argument/result schema in `target`. Pass consecutive
argument child records with their total size, and a disjoint reply buffer
of 1–65536 bytes. Use a reply polling budget of 1–60000 ms. Session I/O
has its own timeouts. The same compiled-interface verifier is used by DisVM.
If the offer uses the managed caller record, publish the supplied nonzero
PID in the reverse tree before the call and keep that publication until
the function returns. An ordinary managed offer needs no reverse PID.

The function writes one checked request on a fresh fid, continuing only
confirmed short writes, then reads and validates its result. The outcome
codes have the [scalar caller's meaning](scalar_invoke.md). Only success
publishes `result`. Its views borrow the reply buffer. For flat reference
arguments the caller applies the validated update mask and records to its
own data. The function exchanges the ordinary managed profile. Graphs,
hybrid tables and channels have their own APIs and framing. Host scheduling,
context ownership and cancellation of the whole session belong to the caller.
The function is synchronous and never replays a write. See the
[standalone C client](../examples/client.c) and [module publisher](managed_module_init.md).
