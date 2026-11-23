# styxrpc_channel_fid_finish

[Русский](channel_fid_finish.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_fid.h>
StyxRpcStatus styxrpc_channel_fid_finish(StyxRpcChannelFid *fid, unsigned selected, const void *value, size_t size);
```

Publish the selected result on one pending fid. INVALID for an unencodable result leaves it pending and permits another encoding attempt. The host must arbitrate readiness and commit only one guard.

C99. Operations on a shared fid are serialized by its owner. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel_fid.c)
