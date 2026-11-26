# Styxnodes.publish

[Русский](styx-nodes-publish.ru.md) · [Guide](STYX-NODES.md) · [Interface](../module/styxnodes.m#L64) · [Implementation](../limbo/styxnodes.b#L101)

```limbo
include "sys.m";
include "styxnodes.m";
publish: fn(path, permissions: string): ref Node;
```

Publish one file in the launcher’s live namespace. `path` is an
absolute UTF-8 path of up to 1023 bytes. Provide an existing writable
and searchable parent directory and an absent final name. Empty,
`.`, `..` and reserved path components are invalid. `permissions` is
a nonempty subset of `rwx` with each letter at most once. Ordering
does not matter. The mode gives the selected permissions to all peers
with access to that namespace.

The result owns the registration and its local fid. This call may wait
for VFS I/O. It raises on invalid parameters, a collision, missing
rights or an unavailable paired owner service. It does not create
parent directories. Serialize calls on the same node after publication.

```limbo
node := nodes->publish("/tools/mail", "rwx");
```

See [init](styx-nodes-init.md), [dispatch](styx-nodes-dispatch.md) and
[close](styx-nodes-close.md).
