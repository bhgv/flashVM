# styxrpc_cell_get

[Русский](cell_get.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L55) · [Implementation](../src/named_channel_client.c#L328)

Read a snapshot of the current frame through an existing cell fid.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_get(StyxRpcVfsFile *lease,
    unsigned char **frame, size_t *size);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `lease` | Open read/write cell fid held by the caller. The function borrows it. |
| `frame` | Writable output pointer. Receives a buffer allocated by `malloc` on success. Release it with `free`. |
| `size` | Writable output for the exact frame length in bytes, including its tag. |

## Return value

On `STYXRPC_IO_OK`, the caller owns the frame and receives its length. Failure keeps both output slots unchanged. Invalid pointers return `STYXRPC_IO_NOT_OPEN`. A malformed, empty or incomplete confirmed reply returns `STYXRPC_IO_FAILED`. A transport without positive acknowledgement can return `STYXRPC_IO_UNCERTAIN`.

## Behavior and lifetime

Serialize requests and close on `lease`. After a failed exchange with a partially consumed reply, retire that fid before using the cell again. Keep a separate lease if the application needs the token to stay valid. Different successful reads may observe different values.

The call can block for I/O under the mounted session's request deadlines.

## Example

Pass the existing cell lease. Each successful read transfers a separate buffer to this caller.

```c
unsigned char *frame = NULL;
size_t size = 0;
if (styxrpc_cell_get(lease, &frame, &size) == STYXRPC_IO_OK)
{
    free(frame);
}
```

Include `<stdlib.h>` for `free`. Decode the snapshot before freeing it when needed.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_read_until](cell_read_until.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
