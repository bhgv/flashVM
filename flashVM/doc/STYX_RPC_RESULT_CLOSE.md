# endpoint.close

[Русский](STYX_RPC_RESULT_CLOSE.ru.md) · [Guide](STYX_RPC_RESULT_CHANNELS.md) · [Implementation](../src/styxrpc/channel_result_endpoint.c#L194)

## Syntax

```actionscript
endpoint.close():Boolean
```

Takes no arguments. Releases the local reference and returns `true` once. Further close operations return `false`. An unpublished resource is freed with the endpoint. An issued resource stays available under the peer's lease until its function fid retires. The call performs local cleanup without network waits. GC performs the same owner-reference release when the object is collected.

```actionscript
var released = owner.close(); // All local aliases observe the same closed state.
```

[Active](STYX_RPC_RESULT_ACTIVE.md) · [C owner release](../../lib/styxrpc/reference/channel_mailbox_owner_release.md)
