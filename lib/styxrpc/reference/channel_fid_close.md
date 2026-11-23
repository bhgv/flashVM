# styxrpc_channel_fid_close

[Русский](channel_fid_close.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/channel_fid.h>
void styxrpc_channel_fid_close(StyxRpcChannelFid *fid);
```

Close an operation. If unresolved, call the scheduler cancellation handler once before buffers can be reused. Serialize close with the host scheduler.

C99. Operations on a shared fid are serialized by its owner. `StyxRpc::rpc`.

[Example](../tests/channel_socket.c) · [Source](../src/channel_fid.c)
