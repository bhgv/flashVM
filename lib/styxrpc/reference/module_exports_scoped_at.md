# styxrpc_module_exports_scoped_at

[Русский](module_exports_scoped_at.ru.md) · [Declaration](../include/styxrpc/module_client.h) ·
[Implementation](../src/client_exports.inc)

```c
#include <styxrpc/module_client.h>

StyxRpcStatus styxrpc_module_exports_scoped_at(struct StyxFid *directory,
    uint32_t pid, StyxRpcModuleExports *out, uint32_t *scoped);
```

Discover the checked contracts on the borrowed unopened `directory` as
in [styxrpc_module_exports_at](module_exports_at.md). A nonzero `pid`
permits imports that require a caller scope. This function checks those
requirements on the same directory as the export names and call files.

On `STYXRPC_OK`, `out` owns its value fields and names. Keep it at its
original address. If `scoped` is supplied, bit `i` reports whether
`out->methods[i]` requires a reverse caller scope. For example,
`scoped & UINT32_C(1)` checks the first export after a successful call.
On `STYXRPC_INVALID`, both outputs retain their prior contents.

The call can wait for Styx I/O. The caller retains the directory and
serializes its close with discovery.
