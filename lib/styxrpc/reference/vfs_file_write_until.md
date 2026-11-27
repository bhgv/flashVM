# styxrpc_vfs_file_write_until

[Русский](vfs_file_write_until.ru.md) · [VFS overview](../README.md)

Write a confirmed prefix on an open file with an optional stop callback.

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_file_write_until(StyxRpcVfsFile *file, uint64_t offset,
    const void *bytes, size_t size, size_t *used, StyxRpcVfsStop stop, void *state);
```

Link the hosted `StyxRpc::launch` target.

| Parameter | Contract |
| --- | --- |
| `file` | Borrow a fid opened for writing with [styxrpc_vfs_open](vfs_open.md) or [styxrpc_vfs_create](vfs_create.md). Serialize transfers and close on this fid. |
| `offset` | First byte to write. The whole requested range must fit in a signed 64-bit Styx offset. |
| `bytes`, `size` | Borrow up to 65,536 readable bytes through return. A zero size allows NULL and skips the data request. |
| `used` | Required output for the confirmed byte count, including on cancellation or transfer failure. |
| `stop`, `state` | Optional callback and borrowed context. Return nonzero promptly to request cancellation. The callback runs on the calling thread and leaves this fid to its caller. |

The SDK advances past each confirmed prefix before writing its suffix.
A NULL callback gives
[styxrpc_vfs_file_write](vfs_file_write.md) behavior. A callback is
checked before each fragment and while waiting for a multiplexed
reply. A serial client checks before each fragment. A transmitted
pending request uses the existing `Tflush` exchange on multiplexed
sessions. A valid `Rwrite` before `Rflush` retains its count.

`STYXRPC_IO_CANCELLED` preserves the confirmed prefix in `used`.
The pending fragment may have changed the file even if its
acknowledgement was lost. Close the cancelled fid with
[styxrpc_vfs_close](vfs_close.md) and resolve the file state before
choosing another write. An unresponsive peer can delay flush until
the session deadline. See [ordinary write outcomes](vfs_write_at.md).

The [wire fixture](../tests/channel_stop_wire.c#L211) writes two
bytes and acknowledges the first. Stopping the second gives
`STYXRPC_IO_CANCELLED` and `used == 1`. Its peer checks that the
second write carries only the suffix at offset one.

[Declaration](../include/styxrpc/vfs_client.h#L122) ·
[Implementation](../src/vfs_client.c#L275)
