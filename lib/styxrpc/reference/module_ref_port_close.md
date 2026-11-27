# styxrpc_module_ref_port_close

[Русский](module_ref_port_close.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h) · [Implementation](../src/module_ref_port.c) · [Guide](module_ref_port.md)

`StyxRpc::launch`

```c
void styxrpc_module_ref_port_close(StyxRpcModuleRefPort *port);
```

Pass any owned `port` returned by [open](module_ref_port_open.md) or [paired open](module_ref_port_open_paired.md), or pass NULL. First finish operations using it. This consumes the handle and its client pin. Any held remote fid is queued for clunk without a synchronous Styx request. A local file is closed through its VFS operations. Other handles keep their own pins.

Close a port after receiving the final value, or after another participant has received its descriptor and this binding is no longer needed.
