# styxrpc_vfs_int_open

[Русский](vfs_int_open.ru.md) · [API](../include/styxrpc/vfs_client.h) · [Source](../src/vfs_client.c) · [Guide](../MODULES.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_vfs_int_open(struct StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out);
```

## Contract

Open a known relative `.var/name` cell path, including a host-authorized path under `.prog`. The SDK checks that the fid refers to a regular four-byte cell. Set `writable` to 0 for read-only or 1 for read/write. Success stores the held fid in `out` and returns `STYXRPC_IO_OK`. A refused path, mode or stat returns `STYXRPC_IO_NOT_OPEN`. Close a successful fid through the [VFS close API](vfs_close.md). Calls use the session deadline.
