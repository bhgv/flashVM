# styxrpc_module_ref_port_read_until

[Русский](module_ref_port_read_until.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h#L77) · [Implementation](../src/module_ref_port.c#L168)

Read one signed Int32 or a fragment of its cached reference frame with a caller stop hook.

## Syntax

```c
#include <styxrpc/module_ref_port.h>

StyxRpcIoOutcome styxrpc_module_ref_port_read_until(StyxRpcModuleRefPort *port, uint64_t offset,
    void *bytes, size_t size, size_t *used, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached session and a live port. Verified with a native Linux64 DisVM and FlashVM pair.

## Parameters

| Name | Description |
| --- | --- |
| `port` | Bound port borrowed through return. |
| `offset` | Byte position. Use 0 for value access and 4 or greater for a descriptor fragment. Positions 1 through 3 are invalid. |
| `bytes` | Borrowed writable output buffer of `size` bytes. NULL is accepted for a zero-size descriptor read. Failure preserves its contents. |
| `size` | Requested extent in bytes. Value access requires exactly 4. Descriptor reads can be shorter and stop at the cached frame's end. |
| `used` | Required byte count output. Four on a successful value read, the confirmed descriptor extent or zero at end of frame. Zero on failure. |
| `stop` | Optional callback. A nonzero result requests stop. NULL uses the ordinary waits. |
| `state` | Borrowed callback context through return. It can be NULL when accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | A complete value, descriptor fragment or end of frame was returned. |
| `STYXRPC_IO_NOT_OPEN` | Invalid port, offset, extent or other argument. Also returned when the original export is unavailable or has a different Qid. |
| `STYXRPC_IO_FAILED` | The exact transfer failed, such as a short four-byte value or failed identity check. |
| `STYXRPC_IO_CANCELLED` | The caller stopped before completion. No value is published. |
| `STYXRPC_IO_UNCERTAIN` | The transfer did not produce a trustworthy reply. The output is preserved. |

## Behavior and lifetime

For a value, the port resolves its original descriptor if needed, reads one four-byte value at offset zero, then copies it to `bytes`. For a frame, the port copies cached bytes and performs no remote I/O. A complete original data reply received ahead of `Rflush` can still win the race with cancellation.

After an interrupted or unconfirmed remote value operation, the port retires its fid and retains the descriptor and frame bytes. On the next value access it compares the entire original Qid.path. A replacement at the same path is refused.

Serialize port operations and close. The stop callback runs on the calling thread and should return promptly. A multiplexed client can flush a pending request by tag. A sequential client samples stop between exchanges. For local port callbacks, the SDK samples at the callback boundary. Cancellation inside them is provider-dependent.

## Example

This example borrows a port that already holds a descriptor. The tutorial supplies the deadline and callback.

```c
unsigned char value[4];
size_t used;
StyxRpcIoOutcome status = styxrpc_module_ref_port_read_until(port, 0,
    value, sizeof(value), &used, deadline_reached, &deadline);
```

## See also

[`styxrpc_module_ref_port_read`](module_ref_port_read.md) · [`styxrpc_module_ref_port_write_until`](module_ref_port_write_until.md) · [`styxrpc_module_ref_port_close`](module_ref_port_close.md) · [Runnable DisVM and FlashVM example](../examples/disvm/REF_PORT_CANCELLATION.md)
