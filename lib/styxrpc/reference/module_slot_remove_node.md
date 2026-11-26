# styxrpc_module_slot_remove_node

[Русский](module_slot_remove_node.ru.md) · [Header](../include/styxrpc/nodes.h) · [Implementation](../src/module_slot.c)

```c
#include <styxrpc/nodes.h>
StyxRpcStatus styxrpc_module_slot_remove_node(StyxRpcModuleSlot *slot,
    const char *path);
```

Link the hosted SDK. Call on the owner of a live `slot` outside any
node transfer or release callback. `path` uses the same root-relative
UTF-8 spelling as [new_node](module_slot_new_node.md), with up to 1024
bytes excluding an optional leading slash. Both sequential and
[positioned](module_slot_new_node_at.md) nodes can be removed.

On `STYXRPC_OK`, the slot retires opens of this exact identity,
removes the file and prunes now-empty implicit directories. It then
calls the registration's release function once, if supplied. Release is
synchronous on the slot owner. A release callback cannot modify the slot.
A walked but unopened fid also becomes stale. A new registration with the
same name receives a different Qid. Open siblings keep working.

`STYXRPC_INVALID` means the tree was not changed. Causes include a null
slot, missing name, directory, invalid path, or a call during a transfer
or release on that slot. The operation does not send any wire requests.
A fresh execute invocation resolves its path when its full request arrives.

For example, `styxrpc_module_slot_remove_node(slot, "/app/page")` releases
one file. Use [clear_nodes](module_slot_clear_nodes.md) at the end of the
movie to withdraw all remaining nodes.
