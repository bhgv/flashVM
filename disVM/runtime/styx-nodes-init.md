# Styxnodes.init

[Русский](styx-nodes-init.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L53) · [Implementation](../limbo/styxnodes.b#L15)

```limbo
include "sys.m";
include "styxnodes.m";
init: fn();
```

Load the runtime `Sys` module. Call once after loading `Styxnodes`
and before publishing or serving any node. The call takes no arguments,
returns no value and opens no registration. Calling it again after a
successful load does not change existing node ownership.

```limbo
nodes := load Styxnodes Styxnodes->PATH;
if (nodes != nil) {
    nodes->init();
}
```

See [publish](styx-nodes-publish.md).
