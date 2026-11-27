# styxrpc_server_step_control

[Русский](server_step_control.ru.md) · [API](../include/styxrpc/control.h#L59) · [Implementation](../src/serve_control.inc) · [Control tree](control_create.md)

```c
int styxrpc_server_step_control(StyxRpcServer *server);
```

Process a cycle on the owner thread of a control tree or module-slot
server. Call between ordinary steps, for example while an application
activation awaits a debugger command. Until the function returns, only
`.state` opens, reads and writes succeed. Name lookup, stat and normal
fid cleanup remain available. The application must use callbacks and
cleanup that are safe while the activation is suspended.

Like the regular blocking server step, this waits for wire work and
returns 1 to continue. It returns 0 for a disconnect, a server error,
an unsupported tree, NULL or attempted recursive dispatch of the same
server. It borrows `server` and uses its existing delegated transport.
