# styxrpc_module_ref_port_write

[Русский](module_ref_port_write.ru.md) · [Declaration](../include/styxrpc/module_ref_port.h) · [Implementation](../src/module_ref_port.c) · [Guide](module_ref_port.md)

`StyxRpc::launch`

```c
StyxRpcIoOutcome styxrpc_module_ref_port_write(StyxRpcModuleRefPort *port,
    uint64_t offset, const void *bytes, size_t size);
```

Pass `offset` zero and a nonempty borrowed buffer `bytes` of `size` bytes. For an unbound port, pass exactly one [capture or adopt request](module_ref_port.md). Capture opens the issuer's live export and records its Qid. Use `STYXRPC_REF_PORT_CAPTURE_LOCAL` in the same format with a [local capture callback](module_ref_port_open_owned.md) to open the local export. Adopt verifies role, scope and wire format and defers the open. Once bound, pass exactly four bytes in signed big-endian Int32 format to replace a writable value.

`STYXRPC_IO_OK` confirms the complete command or replacement. Invalid arguments, a bad frame or a read-only value return `STYXRPC_IO_NOT_OPEN`. A failed or uncertain Styx operation propagates its status. Do not replay an uncertain replacement automatically. Capture and value I/O can wait for the issuer. Serialize writes with reads and closure of the same port.

For capture of a writable export, prefix the path with the two bytes `STYXRPC_REF_PORT_CAPTURE, 1`. Then read the complete descriptor at [offset four](module_ref_port_read.md).
