# styxrpc_module_ref_port_open_paired

[Русский](module_ref_port_open_paired.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h#L36) · [Implementation](../src/module_ref_port.c#L36) · [Guide](module_ref_port.md)

`StyxRpc::launch`

```c
StyxRpcModuleRefPort *styxrpc_module_ref_port_open_paired(struct StyxClient *issuer, unsigned owner,
    const void *scope, StyxRpcModuleRefLocalOpen local, void *context);
```

Pass the attached ordinary mount as `issuer`, its role as `owner`
(`STYXRPC_MODULE_REF_FORWARD` or `STYXRPC_MODULE_REF_REVERSE`), and
the negotiated 16-byte `scope`. The port copies the scope and retains the
client. It borrows the optional resolver `local` and `context` until
close. With `local` set, Adopt accepts the opposite role, then calls
that resolver on the first value access. Capture still uses the ordinary
issuer mount. Pass NULL as `local` for the behavior of [open](module_ref_port_open.md).

The resolver receives `context`, the validated descriptor, and a zeroed
`StyxRpcFile` for the result. Check the exact path, Qid and requested mode.
On `STYXRPC_IO_OK`, transfer one opened file with the permitted access.
On failure return the applicable outcome and leave the result empty.
An opened file must remain bound to the same node until consumed. Its
VFS operations report local I/O failures and must avoid replaying a write.
The resolver and the local file operations can block on the owner.

Creating the port does no Styx I/O and returns the owned unbound port,
or NULL for invalid arguments or allocation or retain failure. Serialize
each port's operations. Use the same [write](module_ref_port_write.md),
[read](module_ref_port_read.md) and [close](module_ref_port_close.md)
contract as for a port with one remote owner.

The [mixed DisVM adapter](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/terminal_launch/module_refs.c)
uses the forward mount as issuer and resolves reverse Qids in its recorded
local publications.
