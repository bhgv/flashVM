# Integer variable in VFS

[Русский](VFS-INT-VARIABLE.ru.md) · [Limbo interface](../module/vfsexport.m) · [Example](fixtures/vfs-publication/main.b)

```limbo
intvar: fn(initial: int): ref Root;
intget: fn(root: ref Root): (int, int);
intset: fn(root: ref Root, value: int): int;
```

Load `Vfsexport` from `Vfsexport->PATH`. `intvar` creates a shared atomic
signed 32-bit cell and returns its exact `Root` or nil on refusal. Share
the root with another Limbo process or use `overlay` to expose it as a file.
`intget` returns `(1, value)` on success and `(0, 0)` for another root.
`intset` returns 1 after a committed update or 0 on refusal. Typed operations
require the original cell reference. File projections retain their
individual read and write restrictions.

The file contains four big-endian bytes. Each open gets its own cursor.
Read or write one whole value at offset zero. Partial writes are refused.
Typed and file operations share one value, with no managed references in
file callbacks. Root capabilities and open descriptors keep the cell alive.

Run `ctest -R '^disvm_native_vfs_publication$' -V`. The example creates
a cell from Limbo, maps it at `/.var/count`, shares it with a child and
checks updates through both APIs. This example verifies local VFS publication.
