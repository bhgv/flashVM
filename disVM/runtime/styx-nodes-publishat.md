# Styxnodes.publishat

[Русский](styx-nodes-publishat.ru.md) · [Guide](STYX-NODE-OFFSETS.md) · [Declaration](../module/styxnodes.m#L68) · [Source](../limbo/styxnodes.b#L108)

```limbo
include "sys.m";
include "styxnodes.m";
publishat: fn(path, permissions: string, length: big): ref Node;
```

Publish an offset-aware node in the launcher’s live namespace.
`path` and `permissions` follow [publish](styx-nodes-publish.md).
`length` is a byte count from zero through 2⁵³−1. It is reported in
VFS metadata and stays fixed until withdrawal. The node owns its
registration fid. Invalid values, an occupied path or publication
failure raise an exception. Serve it on the owner’s thread.

```limbo
file := nodes->publishat("/tools/data", "rw", big 4096);
```

Use [dispatchat](styx-nodes-dispatchat.md) or [recvat](styx-nodes-recvat.md)
to use page offsets. Release it with [close](styx-nodes-close.md).
