# Pass FlashVM channels to DisVM

[Русский](STYX_RPC_CHANNEL_ARGUMENTS.ru.md) · [Live channel results](STYX_RPC_CHANNELS.md) · [Independent C SDK](../../lib/styxrpc/reference/channel_argument_scopes.md)

A scoped Limbo function can receive live FlashVM argument channels while the caller waits for its answer. FlashVM lends native typed queues through libstyxrpc. The host's reverse Styx dispatcher processes operations while the context owner is waiting. The dispatcher never calls an ActionScript method.

## Prepare the host and call from ActionScript

1. Use one bidirectional connection with a forward mount to DisVM and a reverse Styx server. Register the caller PID with [the C program tree](../../lib/styxrpc/reference/program_add.md).
2. Create one [argument bus](../../lib/styxrpc/reference/argument_bus_create.md) for that PID and register its resource as [`.prog/<pid>/.unibus`](../../lib/styxrpc/reference/program_unibus.md) before serving. Keep the bus on this connection until the reverse server stops.
3. Install the [local channel constructor](STYX_RPC_CHANNEL_FACTORY.md) and [argument import](STYX_RPC_CHANNEL_ARGUMENTS_IMPORT.md) on the FlashVM owner. Use the exact compiled arguments, result, signature and PID.
4. Create a typed script channel, place any initial values in it and pass it to the imported function. For example, with an imported `exchange` for `fn(port: chan of int): int`:

```actionscript
var port = newRpcChannel("i", 1);
port.trySend(41);
var observed = exchange(port);
var reply = port.tryReceive();
```

The function can receive from `port` and send back to the same queue before it returns. Limbo also uses ordinary `alt` on admitted channels. Channel positions can occur in value arrays and tuples. A repeated argument maps to one key and one queue. AS `null` represents a nil channel.

## Lifetimes and errors

The SDK verifies the scoped offer, binds the exact PID and keys, then activates the bus immediately before the first write. On completion, timeout or error, it retires the generation and detaches unresolved reverse operations. Queue changes from completed operations remain visible in AS. A remote exception or uncertain timeout therefore does not imply that a send or receive was rolled back. A separately returned DisVM channel can stay live through its own result lease.

Each call admits up to eight distinct queues. Each local queue has zero to eight slots. Capacity zero supports rendezvous between DisVM operations during the call. Only by-value channel elements are bound here. DisVM-to-FlashVM channel arguments and delegation of channels inside elements remain open.

[Executable FlashVM caller](../tests/styxrpc/channel-args.c) · [DisVM owner](../../lib/styxrpc/tests/flashvm/channel_args_owner.b)
