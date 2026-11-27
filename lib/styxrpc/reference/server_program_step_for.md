# styxrpc_server_program_step_for

[Русский](server_program_step_for.ru.md) · [Declaration](../include/styxrpc/serve.h) ·
[Implementation](../src/serve_program_wait.inc)

```c
#include <styxrpc/serve.h>

int styxrpc_server_program_step_for(StyxRpcServer *server, int timeout_ms);
```

Drive one reverse Styx turn while the application owner waits for a
synchronous call. Use a borrowed module-slot server with a registered
native program tree. Its program resources must be safe to call while
the application is suspended. Pass a nonnegative readiness timeout in
milliseconds. Zero permits immediate progress without waiting for
transport readiness.

The native `.prog` tree can be walked and used during this turn. Ordinary
file operations outside that tree are refused. Clunk callbacks outside
the tree are deferred until ordinary dispatch or server destruction.
Transport protocol cleanup remains active.

The return value is one if the dispatcher can continue, including after
a readiness timeout. Zero means link failure, invalid arguments, a
server without a native program tree, or a call from inside another
server dispatch. The call neither takes ownership of the server nor
enters script. Use it on the same owner that runs the ordinary server
loop. Do not step the server concurrently from another thread.

For example, while a native wire job is outstanding, call
`styxrpc_server_program_step_for(server, 5)` between completion checks.
Join the wire job before returning to the application. Resume with
[ordinary server stepping](server_step_for.md).
