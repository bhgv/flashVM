# Styxnodes.recvat

[Русский](styx-nodes-recvat.ru.md) · [Guide](STYX-NODE-OFFSETS.md) · [Declaration](../module/styxnodes.m#L72) · [Source](../limbo/styxnodes.b#L118)

```limbo
include "sys.m";
include "styxnodes.m";
recvat: fn(node: ref Node): (ref Request, big);
```

Wait on the live `node` in its owner thread. Return the same owned
request as [recv](styx-nodes-recv.md) together with its absolute byte
offset. Execute and the original sequential mode return offset zero.
Invalid or closed registrations fail as they do in `recv`. The result
uses the same request ticket and the same reply/refuse operations.

```limbo
(request, offset) := nodes->recvat(file);
```

Reply through [Node.reply](styx-nodes-reply.md) or
[Node.refuse](styx-nodes-refuse.md).
