# styxrpc_channel_fid_write

[Русский](channel_fid_write.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_fid.h>
StyxRpcStatus styxrpc_channel_fid_write(StyxRpcChannelFid *fid, size_t offset, const void *bytes, size_t size);
```

Feed a contiguous byte prefix to a zeroed and configured fid. MORE means a partial frame. OK calls the host scheduler exactly once. The request buffer stays pinned on that fid until close.

C99. Operations on a shared fid are serialized by its owner. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel_fid.c)
