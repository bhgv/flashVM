# styxrpc_module_slot_clear_nodes

[Русский](module_slot_clear_nodes.ru.md) · [Header](../include/styxrpc/nodes.h) · [Implementation](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
void styxrpc_module_slot_clear_nodes(StyxRpcModuleSlot *slot);
```

Withdraw all callback files and their implicit directories. This consumes
each registered callback context through its optional release hook. Previously
opened callback fids fail subsequent transfers and remain available for clunk.
Existing playback controls and module images are independent.

Call on the slot owner outside node callbacks. NULL and repeated calls are
valid. The method performs no transport I/O.
