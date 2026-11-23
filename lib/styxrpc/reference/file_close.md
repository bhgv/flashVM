# styxrpc_file_close

[Русский](file_close.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/vfs.h>
void styxrpc_file_close(StyxRpcFile *file);
```

Release the cookie once and clear caller-owned file. NULL and already-closed state are accepted. The dispatcher calls this when the fid is clunked or the connection drops.

C99. The dispatcher serializes operations on each fid. `StyxRpc::rpc`.

[Example](../examples/device.c) · [Source](../src/vfs.c)
