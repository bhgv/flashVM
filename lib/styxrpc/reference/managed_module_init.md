# styxrpc_managed_module_init

[Русский](managed_module_init.ru.md) · [Header](../include/styxrpc/managed_module.h) · [Implementation](../src/managed_module.c)

Include `styxrpc/managed_module.h` and link `StyxRpc::serve`.

```c
StyxRpcStatus styxrpc_managed_module_init(StyxRpcManagedModule *module,
    const char *name, const StyxRpcManagedExport *exports, size_t count);
```

Initialize caller-owned `module` from 1–8 distinct descriptors. Each supplies
the exact function name, compiled signature, schema and synchronous callback.
`name` is a single path component of at most 255 bytes. On success, publish
`&styxrpc_managed_module_tree` through [styxrpc_server_create](server_create.md)
with the module as context. The resulting path is `mounted/<name>`. Keep the
descriptors, names and context stable through server destruction. An invalid
name, duplicate, signature or schema returns `STYXRPC_INVALID`.

Each function open owns separate request, reply and scratch buffers of up to
64 KiB each. Writes stage one validated request and invoke the callback once
on the dispatcher thread. A callback may complete or return a remote error.
If a callback defers, the dispatcher cancels it and refuses the write. For
asynchronous completion, use the lower-level
[managed open and finish API](managed_open.md). Clunk and disconnect release
the per-fid storage. Callbacks must run on the application owner thread.
