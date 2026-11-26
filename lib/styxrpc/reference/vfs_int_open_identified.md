# styxrpc_vfs_int_open_identified

[Русский](vfs_int_open_identified.ru.md) · [API](../include/styxrpc/vfs_client.h#L138) · [Source](../src/vfs_client.c#L313) · [Guide](module_refs.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_vfs_int_open_identified(struct StyxClient *session,
    const char *path, int writable, StyxRpcVfsFile **out, uint64_t *identity);
```

## Contract

Pass an attached `session` and a relative typed variable `path` such as `mounted/counters.dis/.var/count`. Set `writable` to 0 for read-only or 1 for read/write. Success returns `STYXRPC_IO_OK`, assigns an owned fid to `out` and copies Qid.path from the same stat into `identity` when that pointer is supplied. NULL `identity` gives the behavior of [open](vfs_int_open.md).

Bad arguments or failure to walk or open returns `STYXRPC_IO_NOT_OPEN`. Refused stat or an extent other than four bytes returns `STYXRPC_IO_FAILED`. Both leave the outputs unchanged. The call uses the session deadline for its Styx operations. Release a successful fid with [close](vfs_close.md).
