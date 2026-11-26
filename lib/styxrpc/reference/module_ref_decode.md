# styxrpc_module_ref_decode

[Русский](module_ref_decode.ru.md) · [API](../include/styxrpc/module_ref.h#L43) · [Source](../protocol/module_ref.c#L81) · [Guide](module_refs.md)

`StyxRpc::rpc`

```c
int styxrpc_module_ref_decode(const void *bytes, size_t size, const void *scope,
    StyxRpcModuleRef *out);
```

## Contract

After consuming tag 9, pass the remaining `size` bytes at `bytes` and the expected 16-byte `scope` for this connection. The caller supplies storage for `out`. Success returns 1 and copies a complete descriptor with a NUL-terminated path. A foreign scope, truncated or malformed payload, bad path or NULL argument yields 0 and preserves `out`.

The function retains no arguments and can run on any thread. Select the original owner's [ordinary mount](module_ref_open_remote.md) or [local slot](module_ref_open_local.md) before opening the decoded export.
