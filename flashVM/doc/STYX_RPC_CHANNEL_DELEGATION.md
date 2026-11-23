# Delegate UniBus channels on separate routes

[Русский](STYX_RPC_CHANNEL_DELEGATION.ru.md) · [Nested message channels](STYX_RPC_NESTED_CHANNELS.md) · [Independent C SDK](../../lib/styxrpc/reference/channel_leases.md)

Typed UniBus messages can carry a route beyond the containing channel's current bus. A reverse PID names a resource the host exposes on the peer-facing half of the same Styx connection. A module token addresses a separate publication on the pinned DisVM module. The native Dis provider uses both when exchanging recursive channels.

## Publish an ActionScript queue

1. Initialize the reverse program registry and create a [program server](../../lib/styxrpc/reference/program_server_create.md) on the reverse half of the same bidirectional Styx connection used for the DisVM call. Run the dispatcher on its own thread. On the FlashVM owner, install the [local channel factory](STYX_RPC_CHANNEL_FACTORY.md) as `newRpcChannel` and [the publisher](STYX_RPC_CHANNEL_PUBLISHER.md) as `publishPort`.
2. Call the Dis function and keep a leased channel whose message expects a channel of integers, called `outgoing` here. An ActionScript queue with slots can be seeded before publication.

   ```actionscript
   var queue = newRpcChannel("i", 2);
   queue.trySend(81);
   var port = publishPort(outgoing, queue);
   outgoing.send(port);
   ```

3. Dis opens `.prog/<pid>/.unibus` through its existing reverse mount and operates on the original queue. While it is published, local queue methods raise a busy exception. Close or release all result channels and release the opaque PID values from the issuing family when the exchange is done. The last release revokes the PID, cancels outstanding operation fids and restores use of the local queue with any remaining values. Keep the reverse server alive through retirement.

The [native host API](STYX_RPC_CHANNEL_DELEGATE_LOCAL.md) can publish the same queue explicitly. The [underlying C resource](../../lib/styxrpc/reference/channel_group_reverse_borrow_graph.md) also works without either VM.

### Publish queues inside messages

When a queued element contains a local channel, the publisher follows the channel references already in the FIFO. All reachable queues are lent together under one PID. Each distinct queue receives a key on that PID's bus. Repeating a channel in several slots or message positions preserves its identity. The peer can exchange by-value data on the leaves and send the published channels back into a compatible queue on the same bus. A graph can hold up to eight queues, including its root. A closed or already lent descendant rejects the entire publication.

After revocation, the borrowed graph returns FIFO data to the original queues. The next local access restores the exact references from the returned records. A traced graph keeps the objects alive across collection until that access. In the [native Limbo test](../../lib/styxrpc/tests/flashvm/channel_pid_graph.b), Dis moves a rendezvous from one FIFO into another. The [FlashVM host](../tests/styxrpc/channel-pid-graph.c) checks both aliases and the surviving data after GC.

## Send a PID already registered by the host

1. On the reverse Styx dispatcher, build a [reverse queue resource](../../lib/styxrpc/reference/channel_group_reverse_output.md) or supply equivalent callbacks. Register it with [the PID publication API](../../lib/styxrpc/reference/program_unibus_grant.md). Keep the registration stable through the peer's use of it.
2. Invoke the compiled DisVM function over the forward half of that connection. Keep an open channel from its result. On the FlashVM owner, [bind the reverse PID to that lease](STYX_RPC_CHANNEL_DELEGATE_PID.md) and make the resulting value available to ActionScript.
3. Use that value in a matching message position on a channel from the same lease. For a message consisting of one channel, use `outgoing.send(reversePort)`. The formatter checks the address, key, shape and capacity flag. A value anchored to a different RPC result raises a script exception before sending.

## Admit a delegated module channel

When DisVM sends a module grant to a reverse resource, the host associates that resource with the issuing result. The C dispatcher can use [the route policy](../../lib/styxrpc/reference/channel_read_routed.md) to accept and hand off the frame. On the FlashVM owner, pass the complete frame and an open result channel to [message admission](STYX_RPC_CHANNEL_ADMIT_MESSAGE.md). The C SDK presents each token on the module held by the result. FlashVM decodes the typed value and reuses the same live AS object for a repeated grant. Use the returned channels' `send()`, `receive()` or nonblocking methods. Applications with an already verified route can still use [single-grant admission](STYX_RPC_CHANNEL_ADMIT_MODULE.md).

The [recursive Limbo fixture](../../disVM/examples/remote-scalar/styx-channel-recursive-owner.b) receives a PID from FlashVM, sends its module child to that reverse resource and receives the identical child back. The [FlashVM host](../tests/styxrpc/channel-delegated.c) sends a second PID backed by a two-slot ActionScript queue through that module channel. Dis writes 43, receives the preloaded 81, and reports 81 through the result channel. FlashVM releases the family and reads 43 from the original queue.

Local queue graphs admit exact keys on their own bus. A host-managed resource with an appropriate route policy handles separately addressed PID or module grants. The host delivers received module messages to the owner with the original result as their anchor.

After [building the integrations](../../lib/styxrpc/examples/README.md), run `ctest --test-dir build -R 'channel-(delegated|pid-graph)' --output-on-failure`. Both scenarios run on the native interpreter and JIT.
