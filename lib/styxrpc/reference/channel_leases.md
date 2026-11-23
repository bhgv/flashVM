# Hosted UniBus channel leases

[Русский](channel_leases.ru.md) · [SDK](../README.md) · [Runnable C client](../examples/disvm/routed_client.c)

Use [the scoped channel client](channel_invoke.md) when a Dis function returns a live channel. The incoming frame and distinct routes are checked before the SDK returns an owned reference. A module fid anchors capability operations and a separate function fid owns the remote token. Use [retain](channel_lease_retain.md) for each local user and [release](channel_lease_release.md) when that reference retires.

Use [leased exchange](channel_lease_exchange.md) for routes issued in the original result. For channel grants arriving inside later messages, use the [routed element parser](channel_element_routes.md) and [routed exchange](channel_route_exchange.md), keeping the issuing lease alive and enforcing your application's delegation policy.

A C application can serve scoped factories with [module initialization](channel_module_init.md). The provider's callback returns a channel resource, and the library authenticates the module bus and revokes the token on function clunk. The application supplies storage and operations. The portable [scheduler](scheduler_init.md) provides bounded FIFO and synchronous rendezvous for these operations.
