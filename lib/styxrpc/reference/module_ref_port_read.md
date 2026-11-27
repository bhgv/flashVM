# styxrpc_module_ref_port_read

[Русский](module_ref_port_read.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h#L29) · [Implementation](../src/module_ref_port.c#L109) · [Guide](module_ref_port.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_module_ref_port_read(StyxRpcModuleRefPort *port,
    uint64_t offset, void *bytes, size_t size, size_t *used);
```

Pass a bound `port`, a writable `bytes` buffer of `size` bytes and writable storage in `used`. At `offset` zero request exactly four bytes for the signed Int32. This can open a deferred fid and wait for the issuer. At offset four or higher, read the cached descriptor. This can return a short count or zero at EOF and performs no Styx requests.

`STYXRPC_IO_OK` confirms the returned count. An unbound port, bad argument, or invalid position or size returns `STYXRPC_IO_NOT_OPEN`. Errors from the ordinary mount are propagated. A short value read returns `STYXRPC_IO_FAILED`. The count is initialized to zero when `used` is valid. Buffers are borrowed. Serialize use of the port.

For forwarding, start the read at `STYXRPC_REF_PORT_FRAME` and send the complete byte sequence on the same named channel.
