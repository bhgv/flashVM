# styxrpc_managed_reply

[Русский](managed_reply.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/managed.h>
StyxRpcStatus styxrpc_managed_reply(uint32_t signature, const StyxRpcSchema *schema, const void *request,
    size_t request_size, const void *reply, size_t reply_size, StyxRpcManagedReply *result);
```

Check both complete frames and select changed ref definitions using the exact original request. OK gives views borrowing reply, REMOTE_ERROR means an acknowledged provider error. Commit updates in the caller after validation.

C99. Serialize operations on the same fid. `StyxRpc::rpc`.

[Example](../examples/../tests/managed.c) · [Source](../src/managed.c)
