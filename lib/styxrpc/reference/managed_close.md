# styxrpc_managed_close

[Русский](managed_close.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
void styxrpc_managed_close(StyxRpcManagedCall *call);
```

Retire the fid and invoke cancel at most once if the job was pending. The host maps clunk and disconnect to this operation. Do not use borrowed argument bytes after close.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/../tests/managed.c) · [Source](../src/managed.c)
