# styxrpc_module_ref_open_remote

[Русский](module_ref_open_remote.ru.md) · [API](../include/styxrpc/module_ref.h#L48) · [Source](../src/variable_ref_client.c#L78) · [Guide](module_refs.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_module_ref_open_remote(struct StyxClient *issuer,
    const StyxRpcModuleRef *ref, StyxRpcVfsFile **out);
```

## Contract

Pass the original owner's ordinary VFS mount as `issuer`. Use a descriptor selected for this owner after [decoding](module_ref_decode.md), with a canonical path, nonzero Qid.path and mode 0 for read-only or 1 for read/write. The call opens in that mode and compares Qid.path from the opened fid.

Success returns `STYXRPC_IO_OK` and assigns an owned fid to `out`. Invalid arguments, refusal or a different Qid yield `STYXRPC_IO_NOT_OPEN`. A failed stat or extent check yields `STYXRPC_IO_FAILED`. Both leave `out` unchanged and close any temporary fid. This synchronous call uses the client's request deadlines. Use [VFS integer reads](vfs_int_read.md) and [writes](vfs_int_write.md) on the returned file, then [close it](vfs_close.md).

[Cancellable form](module_ref_open_remote_until.md).
