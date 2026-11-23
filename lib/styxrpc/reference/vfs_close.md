# styxrpc_vfs_close

[Русский](vfs_close.ru.md) · [Source](../src/vfs_client.c)

```c
#include <styxrpc/vfs_client.h>
void styxrpc_vfs_close(StyxRpcVfsFile *file, int deferred);
```

`StyxRpc::launch`

Consume the owned `file`. NULL is allowed. With zero `deferred`, attempt clunk synchronously through the client. With nonzero `deferred`, submit cleanup to the client without waiting for a remote reply. Serialize with other uses of the fid. The pointer is invalid after the call.
