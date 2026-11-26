# Styxnodes.Node.reply

[Русский](styx-nodes-reply.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L43) · [Implementation](../limbo/styxnodes.b#L176)

```limbo
include "sys.m";
include "styxnodes.m";
reply: fn(node: self ref Node, request: ref Request, count: int): int;
```

Complete one received request on the owning node. `count` is the
number of bytes, from zero through the original buffer length. For read
and execute it sends the resulting prefix of `request.buffer`. For
write it acknowledges that many input bytes. The call borrows the
request during the local write and returns one on acceptance, zero for
a bad count, closed node, stale ticket or failed local I/O. A duplicate
reply does not change the previous result.

```limbo
request.buffer[0] = byte 'A';
accepted := node.reply(request, 1);
```

This example assumes a nonempty read buffer from [recv](styx-nodes-recv.md).
Use [refuse](styx-nodes-refuse.md) if processing fails.
