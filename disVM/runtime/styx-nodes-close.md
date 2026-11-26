# Styxnodes.Node.close

[Русский](styx-nodes-close.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L55) · [Implementation](../limbo/styxnodes.b#L239)

```limbo
include "sys.m";
include "styxnodes.m";
close: fn(node: self ref Node);
```

Release the registration fid owned by `node`. This call returns no
value and may be repeated. The final fid release removes this exact
namespace layer and wakes the pending caller with an error when there
was no answer. Close on the owning thread. Do not keep another direct
reference to `node.fd` if withdrawal should occur at close. Separate
application file opens do not keep the registration alive.

```limbo
node.close();
```

See [publish](styx-nodes-publish.md) and the
[paired example](STYX-NODES.md#run-the-paired-check).
