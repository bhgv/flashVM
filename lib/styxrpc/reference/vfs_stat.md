# styxrpc_vfs_stat

[Русский](vfs_stat.ru.md) · [Header](../include/styxrpc/vfs_client.h) · [Source](../src/vfs_client.c) · [styxrpc_vfs_stat_until](vfs_stat_until.md)

Include `styxrpc/vfs_client.h` and link `StyxRpc::launch`.

```c
StyxRpcIoOutcome styxrpc_vfs_stat(struct StyxClient *session, const char *path, StyxRpcFileInfo *info);
```

Resolve path on the borrowed mounted session and stat that exact fid.
`path` is a relative path of at most 1024 bytes. An empty string addresses
the attach root. Within a nonempty path, empty, dot and parent components are rejected. On success `info` receives a byte
length, mode and directory flag. Failure preserves the output. An invalid
path or failed walk returns `STYXRPC_IO_NOT_OPEN`. A failed stat after a
successful walk returns `STYXRPC_IO_FAILED`.

The operation is synchronous and uses Styx session deadlines. Keep the session and storage alive through return. [C acceptance client](../examples/client.c).

Ordinary paths also refuse the reserved `.prog` component at any depth.

Check [reserved names](vfs_service_name.md) during path normalization.
