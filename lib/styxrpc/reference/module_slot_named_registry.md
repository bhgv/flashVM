# styxrpc_module_slot_named_registry

[Русский](module_slot_named_registry.ru.md) · [Header](../include/styxrpc/module_slot.h#L39) · [Implementation](../src/module_slot_named.c) · [Module slot](module_slot.md)

```c
#include <styxrpc/module_slot.h>

StyxRpcNamedRegistry *styxrpc_module_slot_named_registry(StyxRpcModuleSlot *slot);
```

Create an owned registry that borrows a live `slot`. NULL input or
allocation failure returns NULL. The registry admits canonical absolute
paths through the live callback tree. The root and existing traversable
callback directories accept free channel names. It checks for existing
ordinary entries and excludes reserved service paths and the image tree
under `mounted`. A claim is private to the named service. Conflicting
callback publication returns INVALID until the last participant leaves.
Replacing a directory invalidates new joins to its old claim.

Use [named_peer_host](named_peer_host.md) or the
[named endpoint API](named_channel.md) with this registry. The short
namespace checks synchronize with callback publication without invoking
guest code. Existing opened participants keep their handles across
namespace changes. Stop dispatchers, retire their endpoints and destroy
the registry before destroying the borrowed slot. Link `StyxRpc::serve`.
