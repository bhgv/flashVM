# styxrpc_module_ref_encode

[Русский](module_ref_encode.ru.md) · [API](../include/styxrpc/module_ref.h#L38) · [Source](../protocol/module_ref.c#L49) · [Guide](module_refs.md)

`StyxRpc::rpc`

```c
size_t styxrpc_module_ref_encode(const StyxRpcModuleRef *ref, const void *scope,
    void *bytes, size_t capacity);
```

## Contract

Set `ref->node` to the nonzero Qid.path of the opened export, `owner` to `STYXRPC_MODULE_REF_FORWARD` or `STYXRPC_MODULE_REF_REVERSE`, and `writable` to 0 or 1. The descriptor contains a NUL-terminated [canonical path](module_ref_path.md). Pass a negotiated 16-byte `scope` and the writable buffer `bytes` of `capacity` bytes. Keep these inputs separate from the output buffer.

The result is the payload length in bytes. Zero reports invalid input or insufficient capacity and leaves the buffer unchanged. A `STYXRPC_MODULE_REF_WIRE_MAX` buffer fits any valid payload. Prepend the channel tag 9 to send the payload. The function borrows its inputs for the call, performs no I/O and can run on any thread.
