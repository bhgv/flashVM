# Styxnodes.dispatchat

[Русский](styx-nodes-dispatchat.ru.md) · [Guide](STYX-NODE-OFFSETS.md) · [Declaration](../module/styxnodes.m#L76) · [Source](../limbo/styxnodes.b#L218)

```limbo
include "sys.m";
include "styxnodes.m";
dispatchat: fn(node: ref Node, handler: StyxnodePositionedHandler): int;
```

Receive one request on the live `node` and invoke the borrowed
`handler` on the same Limbo thread. The handler receives the path,
operation, buffer and absolute byte offset. Return one if the local
mailbox accepts the reply or refusal, or zero if the reply has
expired or otherwise failed to arrive. An exception or invalid count
from the handler becomes a refusal. A missing handler or receive
failure raises to the application.

```limbo
accepted := nodes->dispatchat(file, handler);
```

See [transfer](styx-nodes-transfer-at.md) and [recvat](styx-nodes-recvat.md).
