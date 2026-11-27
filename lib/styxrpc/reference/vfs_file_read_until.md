# styxrpc_vfs_file_read_until

[Русский](vfs_file_read_until.ru.md) · [VFS overview](../README.md)

Read a confirmed prefix at a byte offset on an open file. A caller can
stop while a data reply is pending.

```c
#include <styxrpc/vfs_client.h>

StyxRpcIoOutcome styxrpc_vfs_file_read_until(StyxRpcVfsFile *file, uint64_t offset,
    void *bytes, size_t size, size_t *used, StyxRpcVfsStop stop, void *state);
```

Link the hosted `StyxRpc::launch` target.

| Parameter | Contract |
| --- | --- |
| `file` | Open readable fid, for example from [styxrpc_vfs_open](vfs_open.md). Serialize transfers and close on this fid. |
| `offset` | First byte to read. The requested range fits in the signed 64-bit Styx offset range. |
| `bytes`, `size` | Writable storage for up to 65,536 requested bytes. A zero length allows NULL and needs no data request. |
| `used` | Required output for the confirmed byte count, including on a short, failed or cancelled read. |
| `stop`, `state` | Optional [callback](../include/styxrpc/vfs_client.h#L38) and borrowed data. Return nonzero to request stop. Return promptly on the calling thread and leave this fid alone. |

Keep the arguments alive until the call returns. With a NULL callback,
use the same behavior as [styxrpc_vfs_file_read](vfs_file_read.md).
Otherwise the client checks stop before each data fragment. A multiplexed
client also checks while awaiting a reply and sends `Tflush` by tag for
an outstanding request. A serial session checks between fragments.

`STYXRPC_IO_OK` reports complete or short confirmed data. A matching
`Rread` received before `Rflush` preserves its bytes. If a later fragment
is cancelled, `STYXRPC_IO_CANCELLED` reports that outcome and `used`
still describes the confirmed prefix. Close a cancelled fid with
[styxrpc_vfs_close](vfs_close.md) before opening again. The provider could
already have consumed external input. A responsive peer must acknowledge
the flush. Other fids on a healthy multiplexed session can proceed.
[Ordinary I/O outcomes](vfs_read_at.md) also apply.

## Example

The wire test opens a file, acknowledges its first byte and withholds the
second. The callback observes a stop flag while the second read is pending.
The expected result is `STYXRPC_IO_CANCELLED` with `used == 1`.
[Test call](../tests/channel_stop_wire.c#L200)

[Declaration](../include/styxrpc/vfs_client.h#L112) ·
[Implementation](../src/vfs_client.c#L267)
