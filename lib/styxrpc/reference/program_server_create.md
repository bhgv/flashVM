# styxrpc_program_server_create

[Русский](program_server_create.ru.md) · [libstyxrpc](../README.md) · [Declaration](../include/styxrpc/program.h#L127) · [Implementation](../src/serve.c#L337)

## Syntax

```c
#include <styxrpc/program.h>
StyxRpcServer *styxrpc_program_server_create(struct RemotePhy *phy, StyxRpcPrograms *programs);
```

Serve a process registry with temporary PID changes allowed from a second thread. Link `StyxRpc::serve` in a hosted SDK build. Prepare `programs` and its caller-owned slots with [initialization](programs_init.md) and any permanent registrations before creating the server. Pass an authorized reliable, ordered Styx stream as `phy` on the reverse half of the connection. The call takes ownership of `phy` on every outcome and borrows the registry and slots until destruction.

Returns the dispatcher or NULL for invalid initialization, allocation, mutex or carrier setup failure. An owner thread runs [server steps](server_step.md). The dispatcher holds a per-server mutex for request processing, with the idle transport wait outside that mutex. Keep the registry's storage fixed and use the locked APIs for any publication or revocation during this period. Resource callbacks on this server must return promptly and may not reenter the same server's registry APIs. Stop callers and finish all grants before handing the server back for [destruction](server_destroy.md).

See the [live C test](../tests/program_server.c) for socket-pair setup and dispatcher shutdown. [Publish a PID](program_server_grant.md) · [Revoke a PID](program_server_revoke.md)
