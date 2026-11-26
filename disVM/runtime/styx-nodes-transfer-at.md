# StyxnodePositionedHandler.transfer

[Русский](styx-nodes-transfer-at.ru.md) · [Guide](STYX-NODE-OFFSETS.md) · [Declaration](../module/styxnodes.m#L15) · [Source](../../lib/styxrpc/tests/flashvm/positioned_handler.b)

```limbo
include "sys.m";
include "styxnodes.m";
transfer: fn(path, operation: string, buffer: array of byte, offset: big): int;
```

Implement this function in the module passed to
[dispatchat](styx-nodes-dispatchat.md). `path` is the registered
absolute name. `operation` is `"r"`, `"w"` or `"x"`. `buffer` is the
mutable request array. Start data reads or writes at the absolute
byte `offset`. `len buffer` is the available capacity for read and
the input count for write or execute. Execute always receives offset
zero. Return a confirmed count from zero through `len buffer`; for
read and execute fill that prefix. A negative count or exception
declines the operation. The callback runs in the owner's Limbo thread.

See the [sparse-file handler](../../lib/styxrpc/tests/flashvm/positioned_handler.b).
