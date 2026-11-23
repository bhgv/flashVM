# styxrpc_vfs_write_at

[Русский](vfs_write_at.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Source](../src/vfs_client.c)

Include `styxrpc/vfs_client.h` and link `StyxRpc::launch`.

```c
StyxRpcIoOutcome styxrpc_vfs_write_at(struct StyxClient *session, const char *path, uint64_t offset, const void *bytes, size_t size, size_t *used);
```

Write up to 65536 caller-owned bytes to an existing writable file
starting at the supplied byte offset on a single fid. Uses the
[read operation's path and outcome rules](vfs_read_at.md). Writes
continue at the confirmed next offset. A confirmed zero-byte write stops
with `STYXRPC_IO_OK`, which may describe a short prefix. On failure
`used` receives only the confirmed prefix. A write with unknown effect
is never replayed. This operation does not create or truncate files.

The operation is synchronous and uses Styx session deadlines. Keep the session and storage alive through return. [C acceptance client](../examples/client.c).
