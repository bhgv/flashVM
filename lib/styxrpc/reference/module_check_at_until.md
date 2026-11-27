# styxrpc_module_check_at_until

[Русский](module_check_at_until.ru.md) · [Declaration](../include/styxrpc/module_client.h) · [Implementation](../src/client_exchange.c)

```c
#include <styxrpc/module_client.h>

StyxRpcStatus styxrpc_module_check_at_until(struct StyxFid *directory,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid,
    StyxRpcModuleStop stop, void *state);
```

The hosted C target `StyxRpc::launch` verifies method contracts on a
borrowed unopened `directory`. Pass `count` entries in `imports`, from
1 through `STYXRPC_MAX_EXPORTS`. Each entry gives a name, codec, signature and
any required shape. Use a valid nonzero caller `pid` if a method needs a
reverse scope. See [the blocking API](module_check_at.md) for descriptor fields.

`STYXRPC_OK` means the interface, required markers and callable
metadata matched. Methods are never invoked. `STYXRPC_INVALID` covers
bad arguments or a refused, missing or mismatched contract.
`STYXRPC_CANCELLED` reports a stop during verification.

The optional `stop(state)` runs on the calling thread and returns nonzero
to stop. A NULL hook uses ordinary blocking waits. Keep state, imports
and directory valid through return. A multiplexed session can flush a
pending walk, open, read or stat by tag. Temporary fid cleanup can
continue after return. Coordinate any directory close with verification.

```c
const StyxRpcModuleImport current = {
    "current", STYXRPC_MODULE_INT, STYXRPC_INT_SIGNATURE, {0}
};
StyxRpcStatus status = styxrpc_module_check_at_until(directory, &current,
    1, 0, stop_requested, task);
```

Take the directory from [live loading](module_live_open_until.md) and
the `current` export from the [test library](../tests/flashvm/live-dis_library.b).
Here `stop_requested(task)` samples the caller's stop flag.
After success use the [checked call API](scalar_invoke_at_until.md).
