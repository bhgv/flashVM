# Styxnodes.Node.recv

[Русский](styx-nodes-recv.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L39) · [Implementation](../limbo/styxnodes.b#L152)

```limbo
include "sys.m";
include "styxnodes.m";
recv: fn(node: self ref Node): ref Request;
```

Wait for the next request on the owning Limbo thread. `node` is a live
registration. The result contains an exact `ticket`, an operation
string (`"r"`, `"w"`, or `"x"`) and an owned byte array up to 65,536
bytes. Read buffers are zero-initialized. Write and execute buffers
contain the incoming fragment. Keep the ticket and operation intact.
While idle, the call yields periodically to the Limbo scheduler.
A closed or unavailable registration raises an exception.

```limbo
request := node.recv();
```

Finish the request with [reply](styx-nodes-reply.md) or
[refuse](styx-nodes-refuse.md).
