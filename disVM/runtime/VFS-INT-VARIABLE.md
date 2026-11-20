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

## Publishing over Styx

A native launcher can pass the cell's exact file provider to
`styxvariableserverstart` from `styx_unibus_server`. The server takes ownership
of the connected channel, retains the exact provider and exposes
`.var/<name>` with the source read and write permissions. Each open fid
has an independent cursor. Call `styxvariableserverjoin` after carrier close.

The [example](../examples/remote-scalar/styx-variable.b) creates a Limbo cell
and sets it to `-2`. An ordinary remote Styx client reads that value and
writes `65539` back. The owner observes the update through `intget`.
The native fixture gives the server the exact provider selected by the
example. Run `ctest -R '^terminal-managed-variable-guest$' -V` in the configured SDK.

Use `styxvariableserverstartprogram` to scope the file to a loaded module.
It accepts the module name and a nonzero PID from the trusted launcher and
publishes `<module>/.prog/<pid>/.var/<name>` and a read-only `.pid` file.
The fixture obtains the PID with `Processctl.pid()`. The receiving side
captures the module directory and opens the cell via
`/.prog/.self/.var/count`. It separately tests the original `.var/count`
layout. See [process context](LAUNCH-PROGRAM-ROUTE.md).
