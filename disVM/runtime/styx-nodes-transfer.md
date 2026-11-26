# StyxnodeHandler.transfer

[Русский](styx-nodes-transfer.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L8) · [Implementation](../../lib/styxrpc/tests/flashvm/nodes_handler.b)

```limbo
include "sys.m";
include "styxnodes.m";
transfer: fn(path, operation: string, buffer: array of byte): int;
```

Implement this function in the Limbo module passed to
[dispatch](styx-nodes-dispatch.md). `path` is the absolute name chosen
at publication. `operation` is `"r"` for a read, `"w"` for a write or
`"x"` for execute. `buffer` is the mutable byte array for this request.
`len buffer` is the read capacity or input length. Fill the prefix
for read and execute. For write, confirm the processed input prefix.
Return the byte count between zero and `len buffer`, or a negative
value to decline. An exception also declines the request. The module
runs on the owner thread, which must finish the reply before making
a synchronous call into the waiting peer.

See the [binary test handler](../../lib/styxrpc/tests/flashvm/nodes_handler.b)
for a working example of all three operations.
