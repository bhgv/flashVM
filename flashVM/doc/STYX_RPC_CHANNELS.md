# Live UniBus channels over Styx RPC

[Русский](STYX_RPC_CHANNELS.ru.md) · [FlashVM–DisVM bridge](STYX_RPC.md) · [Independent C SDK](../../lib/styxrpc/reference/channel_leases.md)

A typed Limbo function can return channels that remain live after the function replies. The transport keeps the issuing function fid open. Each operation opens the module's authenticated `.unibus` on the same mounted connection. The final channel release retires the function fid, which revokes the issued token and pending operations.

## Use DisVM channels from ActionScript

1. Publish the compiled Dis module on the bidirectional Styx connection. Keep the host's delegated caller PID available on the reverse session.
2. Register [the channel import](STYX_RPC_CHANNEL_IMPORT.md) with the mounted session, PID, exact signature and shapes. The client checks the scoped `.interface`, positional `.ref-fields` and `.unibus-version` on the held module before its first write.
3. Call the imported global from AS. Channel references in the result, including in arrays or tuples, become owner-local objects. Repeated references to the same channel share one AS object. A nil channel becomes `null`.

| Method | Result |
| --- | --- |
| `send(value)` | `true` on a confirmed send. Waits for space or for a receiver. |
| `receive()` | One confirmed typed value. Waits for an element. |
| `trySend(value)` | `true` on a confirmed send, `false` on a confirmed idle selection. |
| `tryReceive()` | One typed value or `undefined` on an idle selection. |
| `select(guards)` | One [native selection](STYX_RPC_CHANNEL_SELECT.md) that waits for a send or receive guard on the same bus. |
| `trySelect(guards)` | One immediate selection. An idle reply gives `undefined`. |
| `close()` | Disarms this object and its aliases. Repeated close succeeds. Other distinct channels from the same call keep their lease. |

Methods run synchronously on the FlashVM owner. A timeout, refusal or uncertain transport outcome raises an exception. A timeout can occur after an operation has taken effect. Release discarded objects normally through GC or call `close()` for explicit release.

In a detached task, [`cancel()`](ASINC_CANCEL.md) stops a pending
`send()`, `receive()` (`recv()`) or `select()`. On the paired connection,
the client flushes a pending Styx data request by tag and closes the
operation fid. The route and family
lease remain valid for their other holders. If a request write preceded
cancellation, a matching peer may already have consumed or sent a value.
Releasing the last channel separately retires its issuing lease.

## Use FlashVM channels from Limbo

1. Define an AS initializer and [bind a channel export](STYX_RPC_CHANNEL_EXPORT.md) using the compiled Limbo offer. The initializer receives arguments by value. Its return value supplies one initial element to a FIFO. Returning `undefined` creates an empty channel. Use capacity zero for a rendezvous.
2. Publish the descriptors with [the C channel module](../../lib/styxrpc/reference/channel_module_init.md). The host supplies an entropy callback and serves it on the FlashVM context owner.
3. Mount the provider in DisVM and `load` the compiled interface. Send, receive and `alt` use ordinary Limbo syntax. Each call gets a separate queue or rendezvous and its own token.

The AS bindings support by-value inputs and channel elements, including `int`, counted strings, arrays and tuples. The exporter returns one channel per function call. The import also supports multiple channels in one result. FlashVM can [lend local channels as arguments to DisVM](STYX_RPC_CHANNEL_ARGUMENTS.md). DisVM also lends arguments to [FlashVM channel handlers](STYX_RPC_CHANNEL_ARGUMENTS.md). Those handlers can issue a separate live result channel in the same call. ActionScript can [receive and echo nested Dis-owned channels](STYX_RPC_NESTED_CHANNELS.md) on the same bus. FlashVM can [issue a group of nested result channels](STYX_RPC_NESTED_CHANNELS.md#issue-nested-channels-owned-by-flashvm) under one token. A local ActionScript endpoint for exported result queues remains open. The native C channel APIs continue to expose the full routed protocol.

[DisVM provider fixture](../../lib/styxrpc/tests/flashvm/channel_owner.b) · [Limbo caller fixture](../../lib/styxrpc/tests/flashvm/channel_client.b) · [FlashVM initializer fixture](../tests/styxrpc/channel-service.c)
