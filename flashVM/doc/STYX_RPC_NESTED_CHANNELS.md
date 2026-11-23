# Receive channels inside UniBus messages

[Русский](STYX_RPC_NESTED_CHANNELS.ru.md) · [Channel arguments](STYX_RPC_CHANNEL_ARGUMENTS.md) · [Live results](STYX_RPC_CHANNELS.md) · [C SDK](../../lib/styxrpc/reference/channel_leases.md)

ActionScript can receive a Dis-owned channel in a typed message. The containing channel may come from a DisVM function result or from a DisVM argument to an AS handler. The message can carry a single channel, an array, or a tuple with channel fields. The new key is admitted on the same UniBus route as the receive. DisVM retains ownership of all queues.

## Receive, use and return a message

The [DisVM fixture](../../lib/styxrpc/tests/flashvm/channel_owner.b) exports `nested: fn(value: int): chan of array of chan of int;`. Bind that offer with [the result import](STYX_RPC_CHANNEL_IMPORT.md) and register a script name such as `nestedRPC`. Use the exact compiled signature and the result shape `(c[a[c[i]]])`.

```actionscript
var inbox = nestedRPC(41);
var batch = inbox.receive();
var first = batch[0];
var number = first.receive();
inbox.send(batch);
inbox.close();
first.send(42);
first.close();
```

The fixture puts the same channel into the first two array positions. Both positions give the same AS object, and repeated deliveries reuse that object while it is live. The initial value on that channel is 41. Nil channel positions decode as `null`. An empty `tryReceive()` returns `undefined`.

Sending the array back uses the admitted keys on the original bus. A channel object taken from another call is rejected before the operation is sent. The module token and issuing fid remain live while any admitted result wrapper is open. Closing the containing channel leaves a received child usable. Closing the child closes every AS reference to that same wrapper.

## Use a nested argument in a FlashVM callback

Mount the [FlashVM handler module](../../lib/styxrpc/tests/flashvm/channel_incoming.b) from DisVM and lend a buffered channel with the matching element type. The same `receive()` and `send()` methods work in the handler. The transport checks the received message and admits current-bus keys under the original reverse PID. FlashVM closes every borrowed wrapper at callback exit, including children obtained from later messages. Confirmed sends and receives remain visible in DisVM.

The bridge tracks up to eight distinct keys in one call or lease. This includes the containing channel and any keys discovered in messages. ActionScript supports same-bus round trips for Dis-owned channels. [Host-bound delegation](STYX_RPC_CHANNEL_DELEGATION.md) handles separate reverse PIDs and admitted module tokens. The C SDK exposes the [explicit route policy](../../lib/styxrpc/reference/channel_read_routed.md) for C applications that manage those addresses.

The two directions run in the native Dis interpreter and JIT. After [configuring integration tests](../../lib/styxrpc/examples/README.md), run `ctest --test-dir build -R 'styxrpc-channel(-incoming-dis-to-flash|-flash-to-dis)' --output-on-failure`.

## Issue nested channels owned by FlashVM

An AS initializer can describe a group of new live queues for DisVM. Use [the ordinary channel export](STYX_RPC_CHANNEL_EXPORT.md) or [the handler with borrowed arguments and a channel result](STYX_RPC_CHANNEL_HANDLER_EXPORT.md). The compiled export still returns one outer channel. Its element type can contain channels, including inside arrays or tuples.

For the Limbo offer `tree: fn(seed: int): chan of array of chan of int;`, configure the result shape `(c[a[c[i]]])` and outer capacity one. The initializer returns one seed message:

```actionscript
function makeTree(seed) {
    var data = [2, [seed, seed + 1]];
    var sync = [0, []];
    return [data, data, null, sync];
}
```

In a channel position, `[capacity, items]` describes a new nested queue. `items` is a dense list of initial values, up to the capacity. Each entry follows the compiled element type. A repeated descriptor of the same type issues one shared queue. `null` is a nil channel. A new zero-capacity channel takes an empty list. An outer initializer can also return `undefined` to leave its result channel empty. The whole result group contains up to eight queues including the outer channel, with up to eight FIFO slots per queue.

The module owns these queues and publishes their keys under the result's token on `.unibus`. Limbo can receive the array, use its children, or send the array back while the token is live. Repeated children and rendezvous retain their identity, and a live child keeps the issuing fid after the outer reference is dropped. When that fid closes, the registry retires the whole group. The C SDK builds and owns the buffers through [the group resource](../../lib/styxrpc/reference/channel_group_output.md).

The native fixtures cover a [value-input initializer](../../lib/styxrpc/tests/flashvm/channel_client.b) and a [borrowed-input handler](../../lib/styxrpc/tests/flashvm/channel_incoming.b). Run `ctest --test-dir build -R 'channel-(incoming-)?dis-to-flash|^styxrpc-channel-group$' --output-on-failure` after [configuring integration tests](../../lib/styxrpc/examples/README.md).

## Lend nested FlashVM queues as arguments

Use [the local queue constructor](STYX_RPC_CHANNEL_FACTORY.md) and [the argument import](STYX_RPC_CHANNEL_ARGUMENTS_IMPORT.md). For a Limbo argument of type `chan of array of chan of int`, create the outer queue with element type `"a[c[i]]"`. To exercise the [native fixture](../../lib/styxrpc/tests/flashvm/channel_args_owner.b), bind `nested` with argument shape `(c[a[c[i]]]c[i]i)` and result shape `(i)`.

```actionscript
var batch = newRpcChannel("a[c[i]]", 1);
var item = newRpcChannel("i", 1);
var direct = newRpcChannel("i", 1);
item.trySend(41);
batch.trySend([item, item, null, direct]);
var original = nested(batch, direct, 0);
var again = batch.tryReceive();
var updated = again[0].tryReceive();
```

The fixture returns 41 and enqueues a reply with the same channels. The first two slots share `item`, the fourth is `direct`, and `updated` is 42. The FIFO holds child objects through GC while the message is queued. Each call borrows the direct queues and the open descendants reachable from their queued messages, up to eight distinct queues in total. The SDK gives each queue a fresh scope key and uses the same key whether it appears directly or inside a message. A closed or mismatched child or an oversized graph prevents publication.

On retirement the reverse bus cancels pending operations and revokes every key. FlashVM restores the remaining messages with their original objects, including messages sent back by DisVM. Confirmed queue operations survive an RPC error or timeout. The scope admits only the FlashVM queues captured before the call. Separately addressed channels and new DisVM-owned channels sent into a borrowed argument queue require a separate delegation path.

[Standalone C scope API](../../lib/styxrpc/reference/channel_argument_scopes.md) · [FlashVM fixture](../tests/styxrpc/channel-args.c)
