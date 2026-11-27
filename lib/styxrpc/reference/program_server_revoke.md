# styxrpc_program_server_revoke

[Русский](program_server_revoke.ru.md) · [libstyxrpc](../README.md) · [Declaration](../include/styxrpc/program.h#L141) · [Implementation](../src/serve_grants.inc#L21)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_server_revoke(StyxRpcServer *server, uint32_t pid);
```

Hide the reverse `.unibus` of the exact process `pid` on a [program server](program_server_create.md). A temporary grant also loses its directory. Calls on previously opened fids fail after retirement, and clunk can still release the wire fid. Temporary slots become available for a fresh PID.

Returns 1 if the resource was revoked and 0 for invalid arguments or an absent resource. Open resource handles are closed exactly once before success returns, on the revoking thread. The call can wait for the current request. Run it outside the server's callbacks and keep `server` and the resource callback state alive through completion. Release the resource after revocation. Link `StyxRpc::serve`.

The [live C test](../tests/program_server.c) keeps the old fid open across revocation and checks the replacement independently. [Issue a new PID](program_server_grant.md)
