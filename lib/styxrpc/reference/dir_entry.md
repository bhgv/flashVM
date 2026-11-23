# styxrpc_dir_entry

[Русский](dir_entry.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/vfs.h>
StyxRpcStatus styxrpc_dir_entry(const StyxRpcVfs *ops, void *context, uint64_t directory, size_t index, StyxRpcEntry *entry);
```

Invoke enumeration from ops with borrowed context, parent directory ID and zero-based child index. OK fills entry. Its name lasts until the next enumeration. MORE means end of enumeration. INVALID rejects a bad component or missing callback.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/vfs.c)
