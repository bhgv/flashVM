# styxrpc_module_exports

[Русский](module_exports.ru.md) · [Header](../include/styxrpc/module_client.h) · [Implementation](../src/client_exchange.c)

```c
StyxRpcStatus styxrpc_module_exports(struct StyxClient *session, const char *path,
    uint32_t pid, StyxRpcModuleExports *out);
```

Read the published interface of the visible module at a relative path. The
function holds one walked directory while it reads the profile and verifies the
function nodes. Set `pid` to the caller's reverse PID for scoped profiles, or
zero for an unscoped caller. Successful discovery returns `STYXRPC_OK` and
copies verified imports and owned names to `out`. Keep the output at the
same address while using its name pointers. Unavailable or unsupported
interfaces return `STYXRPC_INVALID` and preserve `out`. The call may block
on the session and runs no exports.
