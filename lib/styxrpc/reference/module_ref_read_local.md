# styxrpc_module_ref_read_local

[Русский](module_ref_read_local.ru.md) · [API](../include/styxrpc/module_ref.h#L60) · [Source](../src/module_ref_local.c#L95) · [Guide](module_refs.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_ref_read_local(StyxRpcFile *file, int32_t *value);
```

## Contract

Pass a held [local export file](module_ref_open_local.md) and the writable location `value` on the slot owner thread. The call reads one complete signed Int32 through the owner's variable callback and returns `STYXRPC_OK` on success. A closed, revoked or refusing file, bad value length or NULL output yields `STYXRPC_INVALID` and preserves the caller's value. No transport request is issued.

After a successful [replacement](module_ref_write_local.md) of the variable's value, another live alias reads the new value. Close the file with `styxrpc_file_close` when finished.
