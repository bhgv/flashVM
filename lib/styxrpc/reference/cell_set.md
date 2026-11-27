# styxrpc_cell_set

[Русский](cell_set.ru.md) · [Declaration](../include/styxrpc/named_cell.h#L59) · [Implementation](../src/named_channel_client.c#L335)

Replace the current frame through an existing cell fid.

## Syntax

```c
#include <styxrpc/named_cell.h>

StyxRpcIoOutcome styxrpc_cell_set(StyxRpcVfsFile *lease, const void *frame, size_t size);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached Styx client and a provider that implements shared cells at `/.channels`. The tutorial is verified on Linux64 with DisVM.

## Parameters

| Name | Description |
| --- | --- |
| `lease` | Open read/write cell fid held by the caller. The function borrows it. |
| `frame` | Borrowed tagged value frame. Readable through return. |
| `size` | Frame length including its tag, 1 through `STYXRPC_NAMED_MAX` bytes. The tag determines further length constraints. |

## Return value

`STYXRPC_IO_OK` confirms the whole replacement. A missing lease or invalid frame returns `STYXRPC_IO_NOT_OPEN`. A definite failure or short confirmed transfer returns `STYXRPC_IO_FAILED`. Missing positive write acknowledgement can return `STYXRPC_IO_UNCERTAIN`.

## Behavior and lifetime

Serialize requests and close on `lease`. A complete accepted body becomes visible atomically. After uncertain acknowledgement, a separate read can show the current value but another holder may have written in the meantime. Retire a fid with an interrupted frame.

The call can block for I/O under the mounted session's request deadlines.

## Example

Pass a live read/write lease. This writes a complete Boolean frame with value false.

```c
const unsigned char disabled[] = {STYXRPC_NAMED_BOOLEAN, 0};
StyxRpcIoOutcome result = styxrpc_cell_set(lease, disabled, sizeof(disabled));
```

A failure can require closing `lease`. Use a separate live lease for recovery if needed.

## See also

[Shared cells](named_cell.md) · [Value frames](cell_value.md) · [styxrpc_cell_write_until](cell_write_until.md) · [styxrpc_vfs_close](vfs_close.md) · [Runnable C and Limbo tutorial](../examples/shared-cell/README.md)
