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

The bridge tracks up to eight distinct keys in one call or lease. This includes the containing channel and any keys discovered in messages. ActionScript supports same-bus round trips for Dis-owned channels. Publishing Flash-owned nested keys or delegations addressed to a separate PID or module token from AS remain open. The C SDK exposes the [explicit route policy](../../lib/styxrpc/reference/channel_read_routed.md) for C applications that manage those addresses.

The two directions run in the native Dis interpreter and JIT. After [configuring integration tests](../../lib/styxrpc/examples/README.md), run `ctest --test-dir build -R 'styxrpc-channel(-incoming-dis-to-flash|-flash-to-dis)' --output-on-failure`.
