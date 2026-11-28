# Compose leased results and retain an ActionScript endpoint

[Русский](STYX_RPC_RESULT_CHANNELS.ru.md) · [Channels](STYX_RPC_CHANNELS.md) · [Runnable AS2, Limbo and C example](../../lib/styxrpc/examples/disvm/CHANNEL_RESULTS.md)


A provider can return a typed tuple or array containing live channels. The same response can return an argument ref and commit changes to it. The result keys all carry one server-issued token. Limbo uses ordinary `load`, receive, send and `alt` on the returned channels.

## Return a graph and a group of channels

Publish a `"channel"` method with the compiler's signature and complete result shape. For `fn(box: ref Box): (ref Box, chan of int, chan of int)` where `Box` has one int field, the argument schema is `(r[i])` and the result schema is `((r[i]c[i]c[i]))`.

```actionscript
function compose(box) {
    box[0] = box[0] + 5;
    var queue = [1, [71]];           // One slot holding an Int32.
    return [box, queue, queue];      // Reuse the same descriptor to preserve identity.
}
```

This uses ordinary AS2 syntax. The runnable example expresses the same arrays with `split` and `push` for the View frontend.

Each channel position in a composite result takes a descriptor `[capacity, items]` or `null`. The descriptor holds 0–8 slots and a dense Array of initial items. A zero-capacity descriptor takes an empty Array. The same descriptor and element type produce the same key. A different type for that descriptor is refused. Up to eight distinct queues fit under one token, including queues in seeded messages. The result and sparse copy-back are checked together before the reply is visible. With only nil channel positions, the copied values and changes travel without a token. [Object publication](STYX_RPC_PUBLISH_OBJECT.md) · [C provider contract](../../lib/styxrpc/reference/channel_module_init.md).

## Retain the other end of a result channel

For a method whose sole result is `c[T]`, create an owner endpoint with [`vfs.resultChannel`](STYX_RPC_RESULT_CHANNEL.md). Give the endpoint back from the method and retain it in the movie to use later.

```actionscript
function buffered(seed) {
    ownerPort = vfs.resultChannel("s", 1);
    ownerPort.trySend(seed);         // Before publication, this can seed a buffered send lane.
    return ownerPort;                // Transfer the publication to the result.
}
```

The peer receives the outgoing value and places each send in a separate incoming lane. The movie polls `tryReceive()` and `trySend()` between calls or frames. With zero slots these methods match a waiting peer operation immediately. A false send requires the application to keep the unsent value if it plans to try again. With a single result the legacy initializer can also return a seed, using the configured capacity.

An endpoint may be returned once, in a sole channel result of the exact element type. Compound results use their own queue descriptors. Use [the PID mailbox API](STYX_RPC_PID_MAILBOX_EXPORT.md) when elements must carry reverse-PID channel grants. A value endpoint accepts copied scalars, strings, dense arrays, tuples and lists admitted by the compiled element type.

After the result fid is clunked, `active()` becomes false. The AS endpoint holds a separate native reference. `close()` releases that reference and any unpublished resource. A published route is owned by the remote lease. [Lifetime and methods](STYX_RPC_RESULT_CHANNEL.md) · [Native example](../../lib/styxrpc/examples/disvm/CHANNEL_RESULTS.md).
