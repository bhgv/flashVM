# styxrpc_channel_fid_read

[Русский](channel_fid_read.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_fid.h>
StyxRpcStatus styxrpc_channel_fid_read(const StyxRpcChannelFid *fid, size_t offset,
    void *bytes, size_t capacity, size_t *size);
```

Read the finished reply at a byte offset. MORE means pending. OK plus zero bytes means EOF. The caller serializes this operation with finish and close.

C99. Operations on a shared fid are serialized by its owner. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel_fid.c)
