# styxrpc_program_server_grant

[Русский](program_server_grant.ru.md) · [libstyxrpc](../README.md) · [Declaration](../include/styxrpc/program.h#L134) · [Implementation](../src/serve.c#L326)

## Syntax

```c
#include <styxrpc/program.h>
uint32_t styxrpc_program_server_grant(StyxRpcServer *server, const StyxRpcResource *resource);
```

On a live [program server](program_server_create.md), add `.prog/<pid>/.pid` and `.prog/<pid>/.unibus`. The server copies `resource`, which must provide read and write callbacks. Keep its context and callback code alive through revocation. Use the returned PID on the same Styx attachment.

Returns a fresh nonzero PID in the upper half of the 32-bit range. Returns zero for invalid input or exhausted slots or PIDs. Three slots are required. Call from the host or dispatcher outside that server's callbacks. The call can wait for a request to finish and does no transport I/O. Keep `server` alive for its duration. Link `StyxRpc::serve`.

The [live C test](../tests/program_server.c) publishes a resource after mounting the peer. [Revoke the grant](program_server_revoke.md) · [Borrow a queue](channel_group_reverse_borrow.md)
