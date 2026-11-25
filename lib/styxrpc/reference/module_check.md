# styxrpc_module_check

[Русский](module_check.ru.md) · [API](../include/styxrpc/module_client.h) · [Source](../src/client_exchange.c) · [Guide](../MODULES.md)

`StyxRpc::launch`

```c
StyxRpcStatus styxrpc_module_check(struct StyxClient *session, const char *module,
    const StyxRpcModuleImport *entries, size_t count, uint32_t pid);
```

## Contract

Borrow an attached session and a relative ordinary module path. Pass one to eight compiled imports in `entries`. The reverse `pid` selects the grant used by the scoped profiles. The call checks signatures, schemas and required markers from the same held directory. It uses the session’s I/O deadlines and issues no function calls. It returns `STYXRPC_OK` when all requested imports match or `STYXRPC_INVALID` for invalid contracts, inaccessible directories or failed verification.

For discovery without an explicit method list, use [module_exports](module_exports.md).
