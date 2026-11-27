# styxrpc_module_slot_programs

[Русский](module_slot_programs.ru.md) ·
[Declaration](../include/styxrpc/module_slot.h) ·
[Implementation](../src/module_slot.c) · [Module slot](module_slot.md)

Include `<styxrpc/module_slot.h>` and link `StyxRpc::serve`.

```c
int styxrpc_module_slot_programs(StyxRpcModuleSlot *slot, StyxRpcPrograms *programs);
```

Attach an initialized [program registry](programs_init.md) to a live
`slot` before starting its Styx dispatcher. `programs` and its storage
remain owned by the application. Keep them alive until the server and
slot are destroyed. A successful first attachment returns 1. An invalid
argument or a second attachment returns 0 and preserves the active tree.

RPC clients can walk directly to `.prog/<pid>`. A root directory read
omits the service. Descendant fids use the registered resources and stay
independent of module image replacement. Make any later registry changes
on the dispatch owner between requests, outside resource callbacks.
The function allocates no storage, waits for no I/O and invokes no callbacks.

[SDK lifetime test](../tests/module_ref.c)
