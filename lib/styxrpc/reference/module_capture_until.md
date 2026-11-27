# styxrpc_module_capture_until

[Русский](module_capture_until.ru.md) · [Declaration](../include/styxrpc/module_client.h) · [Implementation](../src/client_exchange.c)

```c
#include <styxrpc/module_client.h>

StyxRpcIoOutcome styxrpc_module_capture_until(struct StyxClient *session,
    const char *path, struct StyxFid *base, struct StyxFid **out,
    StyxRpcModuleStop stop, void *state);
```

Use the hosted C target `StyxRpc::launch` to capture one mounted module
directory. With a NULL `base`, pass an attached `session` and a relative
`path` in the ordinary VFS. The directory needs read and search permission.
With a non-NULL `base`, pass a borrowed unopened module directory fid.
The function clones that exact fid and ignores `session` and `path`.
This preserves the directory after a path rebind.

On `STYXRPC_IO_OK`, `out` receives an owned unopened fid.
Release it with `styxrpc_vfs_close`. `STYXRPC_IO_NOT_OPEN` covers a bad
argument, a refused walk or an unsuitable directory.
`STYXRPC_IO_FAILED` can indicate a local setup failure.
`STYXRPC_IO_CANCELLED` reports a stop. Failure leaves `out` unchanged.

The optional `stop(state)` runs on the calling thread and returns nonzero
to stop. Keep `state` and `base` valid through return. With a NULL hook
the call uses ordinary blocking waits. Multiplexed sessions flush pending
walk or stat requests by tag. Sequential sessions check stop between
exchanges. Temporary fid cleanup can continue after return.

```c
struct StyxFid *copy = NULL;
if (styxrpc_module_capture_until(NULL, NULL, directory, &copy, NULL, NULL) ==
    STYXRPC_IO_OK) {
    styxrpc_vfs_close(copy, 0);
}
```

Pass the directory returned by [live loading](module_live_open_until.md).
Keep its request lease open while using the clone. The clone can be used
for [export discovery](module_exports_scoped_at_until.md) before close.
