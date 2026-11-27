# styxrpc_module_ref_port_open_owned

[Русский](module_ref_port_open_owned.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h) · [Implementation](../src/module_ref_port.c) · [Guide](module_ref_port.md)

`StyxRpc::launch`

```c
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_owned(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, StyxRpcModuleRefLocalCapture capture, void *context);
```

Pass the attached ordinary mount as `issuer`, its forward or reverse role
as `owner`, and the negotiated 16-byte `scope`. The port retains the
client, copies the scope and borrows both callbacks and their `context`
until [close](module_ref_port_close.md).

`local` resolves an adopted descriptor as described under
[paired open](module_ref_port_open_paired.md). A non-NULL `capture`
requires `local` and enables the first-write command
`STYXRPC_REF_PORT_CAPTURE_LOCAL`. The capture callback receives the
canonical path, requested mode and opposite owner role in `ref`.
It opens that exact local export, sets `ref->node` to its ordinary
issued Qid and returns the owned file in the zeroed output. On failure
it leaves the output empty. Pass NULL for `capture` to disable local
capture. The subsequent read and write operations are the regular
[positioned port](module_ref_port.md) operations.

Construction returns an owned unbound port or NULL on invalid arguments
or allocation or retain failure. Construction performs no transport
I/O. The callback may wait for its provider when a local capture is
submitted. Serialize operations on each port.
