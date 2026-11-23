# Reverse channel argument scopes

[Русский](channel_argument_scopes.ru.md) · [SDK](../README.md) · [Result leases](channel_leases.md) · [Host incoming channel calls](channel_handler_modules.md)

The forward caller owns a typed FIFO or rendezvous for each channel argument. One reverse bus per connection lends these queues to the native Dis provider. The library serializes its reverse operations and admits only the keys of the active scope. Calls sharing the same bus serialize on the application owner.

1. [Create](argument_bus_create.md) the bus for the caller PID. Bind its [resource](argument_bus_resource.md) with [`styxrpc_program_unibus`](program_unibus.md) before the reverse server starts. Keep that dispatcher running while the application waits on the forward client.
2. [Open](argument_scope_open.md) a staging scope and build the typed argument bytes, [encoding each queue](argument_scope_value.md) where its schema expects a channel. Keep storage, keys and element bytes stable while borrowed.
3. [Invoke](argument_scope_invoke.md) with the exact compiled schema and PID. This verifies the offer, then publishes its typed keys before the first write. The library consumes the scope on every outcome.
4. Once the call returns, the original queues have the cursors from completed operations. A returned channel uses the independent [result lease](channel_leases.md). [Close](argument_scope_close.md) discarded staging scopes and [destroy](argument_bus_destroy.md) the bus after server teardown.

Custom forward transports can [activate](argument_scope_activate.md) and close the scope themselves. The [C regression](../tests/channel_arguments.c) verifies that retirement cancels a pending receive, detaches its fid and rejects its old key in subsequent calls.
