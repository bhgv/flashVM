# vfs.publishMessages

[Русский](STYX_RPC_PUBLISH_MESSAGES.ru.md) · [Open a published root](STYX_RPC_OPEN_MESSAGES.md) · [Implementation](../src/styxrpc/session_publish_messages.c#L183)

```actionscript
function onSample(bytes, published) {
    published.send("app.sample", 1, bytes);
}
var published = vfs.publishMessages("samples", _root, ["app.sample", 1, 47, "onSample"]);
```

The paired player publishes a new service-only child in its local Styx server.
Pass one root component, a receiver object and one to 32
[type descriptors](STYX_RPC_OPEN_MESSAGES.md). The peer opens `samples` through
the existing connection. `os.publishMessages` refers to the same method.
Names are relative to the local connection root and independent of cwd.
Ordinary file operations hide these roots. The built-in `.unibus` and `.rpc`
remain available. A duplicate root, existing file or channel, invalid descriptor
or exhausted registry raises an error. The server permits 16 roots in total
and eight open streams per root.

Each complete, checked incoming record invokes the named method on the
receiver as `receiver.method(bytes, published)`. The player runs this callback
synchronously on its owner thread while serving the peer's request. It copies
payload bytes into a private Array. Exceptions reject that peer's stream.
Keep handlers short and avoid waiting for the peer in a handler.

| Method | Effect |
| --- | --- |
| `published.send(name, version, bytes)` | Queue a copy of up to 16,384 bytes on the peer stream invoking this root's handler. Call only within that handler. Returns 0 on success, 1 for full queue, or −1 for a rejected send. The peer reads the response on its next pump. |
| `published.close()` | Retire the root and all its streams. Repeated close succeeds. Call outside message callbacks. |

The movie retains the publication and its receiver until close, movie
replacement, restart or disconnect. Old fids remain retired if a new
publication uses the same name. A new peer opens a fresh stream.

The project-compiled peer check in
[check_flash_peers.py](../../ide/tests/check_flash_peers.py) accepts
`--messages-only`. It tests two independent named roots, a non-root cwd and
binary replies in interpreter and JIT modes. Add `--tcp` for TCP.

[C SDK publish](../../lib/styxrpc/reference/messages_publish.md) · [C SDK withdrawal](../../lib/styxrpc/reference/messages_withdraw.md)
