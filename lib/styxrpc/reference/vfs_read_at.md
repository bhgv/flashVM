# styxrpc_vfs_read_at

[Русский](vfs_read_at.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Source](../src/vfs_client.c)

Include `styxrpc/vfs_client.h` and link `StyxRpc::launch`.

```c
StyxRpcIoOutcome styxrpc_vfs_read_at(struct StyxClient *session, const char *path, uint64_t offset, void *bytes, size_t size, size_t *used);
```

Read up to 65536 bytes in one operation, starting at the specified
byte offset in an existing non-directory node. `path` uses the
[relative path rules](vfs_stat.md). A zero-size transfer checks admission.
The operation holds the same file fid until the requested bytes, EOF or
a failure. When `used` is supplied it reports the confirmed prefix
even if a later read fails. A short result with `STYXRPC_IO_OK` is valid.
Invalid arguments and failures before data I/O return `STYXRPC_IO_NOT_OPEN`.
A confirmed I/O error returns `STYXRPC_IO_FAILED`; an unreliable
acknowledgement returns `STYXRPC_IO_UNCERTAIN`. Never includes
unconfirmed bytes in `used`.

The operation is synchronous and uses Styx session deadlines. Keep the session and storage alive through return. [C acceptance client](../examples/client.c).
