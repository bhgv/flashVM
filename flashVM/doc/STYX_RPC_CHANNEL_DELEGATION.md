# Delegate UniBus channels on separate routes

[Русский](STYX_RPC_CHANNEL_DELEGATION.ru.md) · [Nested message channels](STYX_RPC_NESTED_CHANNELS.md) · [Independent C SDK](../../lib/styxrpc/reference/channel_leases.md)

Typed UniBus messages can carry a route beyond the containing channel's current bus. A reverse PID names a resource the host exposes on the peer-facing half of the same Styx connection. A module token addresses a separate publication on the pinned DisVM module. The native Dis provider uses both when exchanging recursive channels.

## Publish a reverse PID and send it from ActionScript

1. On the reverse Styx dispatcher, build a [reverse queue resource](../../lib/styxrpc/reference/channel_group_reverse_output.md) or supply equivalent callbacks. Register it with [the PID publication API](../../lib/styxrpc/reference/program_unibus_grant.md). Keep the registration stable through the peer's use of it.
2. Invoke the compiled DisVM function over the forward half of that connection. Keep an open channel from its result. On the FlashVM owner, [bind the reverse PID to that lease](STYX_RPC_CHANNEL_DELEGATE_PID.md) and make the resulting value available to ActionScript.
3. Use that value in a matching message position on a channel from the same lease. For a message consisting of one channel, use `outgoing.send(reversePort)`. The formatter checks the address, key, shape and capacity flag. A value anchored to a different RPC result raises a script exception before sending.

## Admit a delegated module channel

When DisVM sends a module grant to a reverse resource, the host checks the incoming message and ties it to the issuing result. The C dispatcher can use [the route policy](../../lib/styxrpc/reference/channel_read_routed.md) and [typed element parser](../../lib/styxrpc/reference/channel_element_routes.md). Pass the verified module route and a live result channel to [module admission](STYX_RPC_CHANNEL_ADMIT_MODULE.md) on the FlashVM owner. Use the returned AS object's `send()`, `receive()` or nonblocking methods. The SDK uses the original module fid, presents the exact token, and reuses the same live AS object for a repeated grant.

The [recursive Limbo fixture](../../disVM/examples/remote-scalar/styx-channel-recursive-owner.b) receives a PID from FlashVM, sends its module child to that reverse resource and receives the identical child back. A second PID offered through the received module channel then exchanges 43 and 81. The scenario uses [one executable FlashVM host](../tests/styxrpc/channel-delegated.c).

The host stays responsible for authorizing the reverse resource and associating a received module grant with the original lease. FlashVM handles typed message encoding and pinned module operations. Automatic publication of arbitrary AS queues as separate PIDs and automatic admission of unbound module tokens remain open.

After [building the integrations](../../lib/styxrpc/examples/README.md), run `ctest --test-dir build -R channel-delegated --output-on-failure`. The two cases run the native interpreter and JIT.
