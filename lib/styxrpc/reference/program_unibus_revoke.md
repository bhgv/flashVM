# styxrpc_program_unibus_revoke

[Русский](program_unibus_revoke.ru.md) · [libstyxrpc](../README.md)

## Syntax

```c
#include <styxrpc/program.h>
int styxrpc_program_unibus_revoke(StyxRpcPrograms *programs, uint32_t pid);
```

Revoke the duplex `.unibus` registered for `pid` in the initialized
`programs` tree. The call hides the file, updates the directory revision
and synchronously invokes the close callback once for each active open.
With a channel fid close callback, pending selections are cancelled before
the function returns. Already committed operations keep their effects.

Old fids reject new reads, writes and opens. Later clunks release the
Styx wrapper without invoking the application callback again. For a temporary
grant the whole directory disappears and its slots are released. Its PID
remains reserved for the session. For a regular `.unibus` the bus slot stays
reserved and other files in the process directory keep their normal access.
The application may free the revoked resource's callback state when this
call returns if no other resource shares it.

Returns 1 for the first revocation. Returns 0 for a missing process or
bus, a repeated revocation or invalid registry state. Use the owning
dispatcher outside callbacks of the target resource. Close callbacks
execute inline and determine whether the call blocks. Link `StyxRpc::serve`.

[Publication](program_unibus.md) · [Socket test](../tests/channel_socket.c) · [Source](../src/program_resource.inc)

[`styxrpc_program_unibus_grant`](program_unibus_grant.md)
