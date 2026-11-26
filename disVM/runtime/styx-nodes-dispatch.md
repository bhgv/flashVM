# Styxnodes.Node.dispatch

[Русский](styx-nodes-dispatch.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L44) · [Implementation](../limbo/styxnodes.b#L153)

```limbo
include "sys.m";
include "styxnodes.m";
dispatch: fn(node: self ref Node, handler: StyxnodeHandler): int;
```

Wait for one request on `node` and call the borrowed `handler`
module synchronously on the current Limbo thread. Return one if the
result or refusal was accepted by the owner service, zero when the
reply could not be delivered. A callback exception or invalid count
becomes a refusal. A missing handler or receive error raises to the
application. For continued service call again on the same owning thread.

```limbo
if (!node.dispatch(handler)) {
    sys->print("reply expired\n");
}
```

See [transfer](styx-nodes-transfer.md) and [recv](styx-nodes-recv.md).
