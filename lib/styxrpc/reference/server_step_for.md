# styxrpc_server_step_for

[Русский](server_step_for.ru.md) · [Header](../include/styxrpc/serve.h) · [Source](../src/serve.c) · [Blocking step](server_step.md)

Include `styxrpc/serve.h` and link `StyxRpc::serve`.

```c
int styxrpc_server_step_for(StyxRpcServer *server, int timeout_ms);
```

Pass the live server and the monotonic wait budget in milliseconds. `-1` waits indefinitely, zero attempts ready I/O without waiting, and a positive value is an upper budget for readiness waits across partial frames. Each call enters the same dispatcher, including buffered requests and replies. A timeout is a successful cycle and returns 1, as does handled readiness or a native wake. A disconnect, transport error or invalid argument returns 0. Callback time is independent of the wait budget. Only the native owner thread calls this function. An event loop may call it with its next timer deadline and run timer work before the next cycle.
