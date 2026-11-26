# Styxnodes.Node.refuse

[Русский](styx-nodes-refuse.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L40) · [Implementation](../limbo/styxnodes.b#L146)

```limbo
include "sys.m";
include "styxnodes.m";
refuse: fn(node: self ref Node, request: ref Request): int;
```

Decline the exact pending `request` received on `node`. The local
write borrows the request and sends its ticket. It returns one if the
owner service accepted the refusal, zero for a closed node, stale
ticket or I/O failure. The waiting caller receives an error.

```limbo
accepted := node.refuse(request);
```

See [recv](styx-nodes-recv.md) and [reply](styx-nodes-reply.md).
