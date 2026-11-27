# styxrpc_module_ref_port_write_until

[Русский](module_ref_port_write_until.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h#L85) · [Implementation](../src/module_ref_port.c#L238)

Bind a positioned reference port or replace its signed Int32 with a caller stop hook.

## Syntax

```c
#include <styxrpc/module_ref_port.h>

StyxRpcIoOutcome styxrpc_module_ref_port_write_until(StyxRpcModuleRefPort *port, uint64_t offset,
    const void *bytes, size_t size, StyxRpcVfsStop stop, void *state);
```

## Availability

Hosted C SDK, linking `StyxRpc::launch`. Requires an attached session and a live port. Verified with a native Linux64 DisVM and FlashVM pair.

## Parameters

| Name | Description |
| --- | --- |
| `port` | Borrowed port. An unbound port accepts a command. A bound port needs writable access for replacement. |
| `offset` | Byte position. Must be zero for both commands and assignments. |
| `bytes` | Borrowed readable buffer. For a command, pass the full coded command in one API call. For a value, pass four bytes in signed big-endian order. |
| `size` | Nonzero byte count. For capture it is two bytes plus the 1–1024 byte canonical relative path with no NUL. For adoption it is one command byte plus the complete named descriptor frame. For a bound port it is exactly 4. |
| `stop` | Optional callback. A nonzero result requests stop. NULL uses the ordinary waits. |
| `state` | Borrowed callback context through return. It can be NULL when accepted by the callback. |

## Return value

| Outcome | Condition |
| --- | --- |
| `STYXRPC_IO_OK` | The complete command or replacement was accepted. The port owns the binding on success. |
| `STYXRPC_IO_NOT_OPEN` | Invalid command or input, read-only mode, unavailable issuer, or a stale Qid. |
| `STYXRPC_IO_FAILED` | A confirmed failure, such as a short value acknowledgement or unsuccessful metadata verification. |
| `STYXRPC_IO_CANCELLED` | The caller stopped during binding or before a value write could affect the peer. |
| `STYXRPC_IO_UNCERTAIN` | The write may have taken effect. Its full acknowledgement is unavailable. The port retains its descriptor. |

## Behavior and lifetime

Before binding, opcode `STYXRPC_REF_PORT_CAPTURE` opens the original remote export and records its Qid. Opcode `STYXRPC_REF_PORT_ADOPT` validates the tagged scoped frame and defers remote open to the first value access. Opcode `STYXRPC_REF_PORT_CAPTURE_LOCAL` uses the embedding's local capture callback. After binding, a value update sends one four-byte write at offset zero. A full original acknowledgement can win the race with `Rflush`.

After an interrupted or unconfirmed remote value operation, the port retires its fid and retains the descriptor and frame bytes. On the next value access it compares the entire original Qid.path. A replacement at the same path is refused. A cancelled capture leaves the port unbound. After an uncertain write, a separate read can report the current value. Avoid automatically replaying the write.

Serialize port operations and close. The stop callback runs on the calling thread and should return promptly. A multiplexed client can flush a pending request by tag. A sequential client samples stop between exchanges. For local port callbacks, the SDK samples at the callback boundary. Cancellation inside them is provider-dependent.

## Example

The port already holds a writable reference. These bytes encode −73.

```c
const unsigned char value[] = {0xff, 0xff, 0xff, 0xb7};
StyxRpcIoOutcome status = styxrpc_module_ref_port_write_until(port, 0,
    value, sizeof(value), deadline_reached, &deadline);
```

## See also

[`styxrpc_module_ref_port_write`](module_ref_port_write.md) · [`styxrpc_module_ref_port_read_until`](module_ref_port_read_until.md) · [`styxrpc_module_ref_port_close`](module_ref_port_close.md) · [Runnable DisVM and FlashVM example](../examples/disvm/REF_PORT_CANCELLATION.md)
