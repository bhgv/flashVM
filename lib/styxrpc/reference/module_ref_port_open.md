# styxrpc_module_ref_port_open

[Русский](module_ref_port_open.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h) · [Implementation](../src/module_ref_port.c) · [Guide](module_ref_port.md)

`StyxRpc::launch`

```c
StyxRpcModuleRefPort *styxrpc_module_ref_port_open(struct StyxClient *issuer,
    unsigned owner, const void *scope);
```

Pass the already attached ordinary VFS client as `issuer`. Set `owner` to `STYXRPC_MODULE_REF_FORWARD` or `STYXRPC_MODULE_REF_REVERSE` for that client's publication. Pass the negotiated 16-byte buffer as `scope`. The port copies it and retains its own client reference. The call performs no Styx requests.

Success returns an owned, unbound port. Invalid arguments or failure to allocate or retain return NULL. Bind it with one [write](module_ref_port_write.md), then use [read](module_ref_port_read.md) and [close](module_ref_port_close.md). In a default DisVM pair the client points to the forward FlashVM. Serialize operations on any one port.
