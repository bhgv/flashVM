# styxrpc_module_exports_scoped_at_until

[Русский](module_exports_scoped_at_until.ru.md) · [Declaration](../include/styxrpc/module_client.h) · [Implementation](../src/client_exports.inc)

```c
#include <styxrpc/module_client.h>

StyxRpcStatus styxrpc_module_exports_scoped_at_until(struct StyxFid *directory,
    uint32_t pid, StyxRpcModuleExports *out, uint32_t *scoped,
    StyxRpcModuleStop stop, void *state);
```

The hosted C target `StyxRpc::launch` discovers methods on the borrowed
unopened `directory` by checking the whole interface, required markers
and callable file metadata. Use a valid nonzero caller `pid` if methods
require a reverse scope.

On `STYXRPC_OK`, `out` owns the names and contracts. Keep it at its
original address because the method names point into the structure. If
`scoped` is supplied, bit `i` reports whether `out->methods[i]` needs
a reverse scope. Use NULL to omit the mask. `STYXRPC_INVALID` covers a
refused, unsupported or empty interface and invalid parameters.
`STYXRPC_CANCELLED` reports a stop. On either failure both outputs keep
their prior values.

The optional `stop(state)` runs on the calling thread and returns nonzero
to stop. Keep `state` and the directory alive through return. A NULL hook
uses ordinary blocking waits. A multiplexed session can flush a pending
exchange by its tag. Temporary cleanup can continue after return.
Concurrent calls need separate outputs and coordination with directory close.

```c
StyxRpcModuleExports exports;
uint32_t scoped;
StyxRpcStatus status = styxrpc_module_exports_scoped_at_until(directory,
    pid, &exports, &scoped, stop_requested, task);
```

Pass a [captured directory](module_capture_until.md).
Here `stop_requested(task)` samples the caller's stop flag.
After success, choose a codec from the discovered contracts or pass them
to [import verification](module_check_at_until.md).
