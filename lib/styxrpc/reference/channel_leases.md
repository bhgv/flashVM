# Hosted UniBus channel leases

[Русский](channel_leases.ru.md) · [SDK](../README.md) · [Runnable C client](../examples/disvm/routed_client.c)

Use [the scoped channel client](channel_invoke.md) when a Dis function returns a live channel. The incoming frame and distinct routes are checked before the SDK returns an owned reference. A module fid anchors capability operations and a separate function fid owns the remote token. Use [retain](channel_lease_retain.md) for each local user and [release](channel_lease_release.md) when that reference retires.

Use [leased exchange](channel_lease_exchange.md) for routes issued in the original result. After a confirmed typed receive, [import current-bus keys](channel_lease_import_element.md) and resolve their [indices](channel_lease_route.md). They share the parent token and issuing fid, so a nested key keeps working while the caller retains the lease. For a message with a separately addressed grant, use the [routed element parser](channel_element_routes.md) and [routed exchange](channel_route_exchange.md) with your application’s delegation policy. If the application verifies a module grant for a particular live result, [admit it into the lease](channel_lease_admit_module.md) to preserve the exact pinned module.

A C application can serve scoped factories with [module initialization](channel_module_init.md). The provider's callback returns a channel resource, and the library authenticates the module bus and revokes the token on function clunk. The application supplies storage and operations or uses [a copied group resource](channel_group_output.md) for initialized nested queues. The portable [scheduler](scheduler_init.md) provides bounded FIFO and synchronous rendezvous for these operations.

For a complete reverse message delivered for this result, use [message import](channel_lease_import_message.md). It confirms each module token on the pinned module and appends routes only after every token succeeds.

A [reverse mailbox](channel_mailbox_create.md) provides separate bounded FIFOs for the peer and the owner on one PID. The dispatcher validates the typed frames. The owner can dequeue a frame and import its module grants under the pinned lease.

A [module-result mailbox](channel_mailbox_result_create.md) accepts the caller's reverse PIDs. The owner [imports the confirmed PID identities](channel_pid_import.md) and uses routed exchange on the same attachment. Clunk retires the resource, and the owner releases its handle separately.
