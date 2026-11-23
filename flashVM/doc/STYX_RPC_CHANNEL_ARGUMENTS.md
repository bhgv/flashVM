# Channel arguments over Styx RPC

[Русский](STYX_RPC_CHANNEL_ARGUMENTS.ru.md) · [Live channel results](STYX_RPC_CHANNELS.md) · [Independent C SDK](../../lib/styxrpc/reference/channel_argument_scopes.md)

## Pass FlashVM channels to DisVM

A scoped Limbo function can receive live FlashVM argument channels while the caller waits for its answer. FlashVM lends native typed queues through libstyxrpc. The host's reverse Styx dispatcher processes operations while the context owner is waiting. The dispatcher never calls an ActionScript method.

### Prepare the host and call from ActionScript

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

### Lifetimes and errors

The SDK verifies the scoped offer, binds the exact PID and keys, then activates the bus immediately before the first write. On completion, timeout or error, it retires the generation and detaches unresolved reverse operations. Queue changes from completed operations remain visible in AS. A remote exception or uncertain timeout therefore does not imply that a send or receive was rolled back. A separately returned DisVM channel can stay live through its own result lease.

Each call admits up to eight distinct queues. Each local queue has zero to eight slots. Capacity zero supports rendezvous between DisVM operations during the call. Flash-owned argument queues carry copied values and [nested channels from the same call scope](STYX_RPC_NESTED_CHANNELS.md#lend-nested-flashvm-queues-as-arguments).

[Executable FlashVM caller](../tests/styxrpc/channel-args.c) · [DisVM owner](../../lib/styxrpc/tests/flashvm/channel_args_owner.b)

## Pass DisVM channels to FlashVM

1. On the FlashVM owner, define the AS function and use [the handler binding](STYX_RPC_CHANNEL_HANDLER.md) to register the exact compiled Limbo offer with its schema. For example, let AS implement `relay(p)` as `return p.receive();`.
2. Create the [hosted C handler module](../../lib/styxrpc/reference/channel_handlers_create.md). Mount the reverse Styx client on the same bidirectional session. In a separate dispatcher, serve the forward tree. On the FlashVM owner, [run the callbacks](../../lib/styxrpc/reference/channel_handlers_step.md). The dispatcher acknowledges the complete write and permits polling while the AS callback uses the reverse bus.
3. Mount the forward tree in native DisVM with `--styx-mount-fd FD --at /rpc` and `load Incoming "/rpc/mounted/flash-incoming.dis"` using the [compiled module declaration](../../lib/styxrpc/tests/flashvm/channel_incoming.m). Start a second Dis process to send a value if the port has zero capacity, then call `api->relay(port)`.
4. At disconnection, release the server and [stop the callback queue](../../lib/styxrpc/reference/channel_handlers_stop.md). Once both threads have returned, destroy the module and release the reverse client.

AS uses [the same channel methods](STYX_RPC_CHANNELS.md#use-disvm-channels-from-actionscript). Repeated references to a channel in value arrays or tuples share the same object during the callback. Nil becomes `null`. A saved AS reference is closed by the time the callback returns or throws. A later call with the same Limbo channel receives a fresh object. Closing the borrowed object affects every alias of that object and leaves DisVM's channel open.

The incoming binding accepts up to eight distinct keys, including [children received on the same bus](STYX_RPC_NESTED_CHANNELS.md). A completed send or receive is visible even if the callback later fails. An operation that exceeds its wait is clunked and raises a script exception.

### Return a separate live channel in the same call

Use [the result binding](STYX_RPC_CHANNEL_HANDLER_EXPORT.md) for an offer such as `make: fn(port: chan of int): chan of int;` and publish it through [the SDK module with a result registry](../../lib/styxrpc/reference/channel_handlers_create_with_results.md). For a result FIFO of capacity one, the callback can use the received value as its seed.

```actionscript
function makeResult(port) {
    return port.receive();
}
```

```limbo
port := chan[1] of int;
port <-= 41;
result := api->make(port);
initial := <-result;
```

`initial` is 41. The caller can use `result` as a regular channel after `make` returns. The temporary ActionScript argument and the new result have separate lifetimes. Each result fid issues a separate token on the module's `.unibus`. The dispatcher revokes its operations on clunk. Choose capacity zero and return `undefined` for an empty rendezvous.

Run the two native integration tests from the installed-SDK build with `ctest --test-dir build -R channel-incoming --output-on-failure`. They run DisVM in interpreter and JIT modes and cover FIFO send/receive, strings containing NUL, aliases, nil, rendezvous, timeout, explicit close, scope retirement and independent result leases.

[FlashVM callback and host](../tests/styxrpc/channel-incoming.c) · [Executable Limbo caller](../../lib/styxrpc/tests/flashvm/channel_incoming.b) · [Configuring the native tests](../../lib/styxrpc/examples/README.md)
