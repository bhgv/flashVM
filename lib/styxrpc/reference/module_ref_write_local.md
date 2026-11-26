# styxrpc_module_ref_write_local

[Русский](module_ref_write_local.ru.md) · [API](../include/styxrpc/module_ref.h#L61) · [Source](../src/module_ref_local.c#L109) · [Guide](module_refs.md)

`StyxRpc::serve`

```c
StyxRpcStatus styxrpc_module_ref_write_local(StyxRpcFile *file, int32_t value);
```

## Contract

Pass a writable [local export file](module_ref_open_local.md) and a signed Int32 `value` on the slot owner thread. The call transfers one replacement at offset zero to the variable callback. Success returns `STYXRPC_OK`. A closed, revoked, read-only or refusing file yields `STYXRPC_INVALID`. The call performs no transport I/O and leaves ownership of `file` with the caller.

Write a value such as `-73`. A later [read](module_ref_read_local.md) can observe it through any live alias of this export.
