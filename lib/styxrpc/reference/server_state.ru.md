# styxrpc_server_state

[English](server_state.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/serve.h>
int styxrpc_server_state(StyxRpcServer *server, StyxRpcServerState *state);
```

Читает снимок текущего числа клиентов, байтов в очереди ответов и свободных мест в очереди. Сеть не опрашивается, счётчики в обработку запросов не добавляются. Возвращает 1 при успехе и 0 при NULL. Вызывается владельцем между шагами или из колбэка.

Hosted Linux64. `StyxRpc::serve`.

[Пример](../tests/directory.c) · [Исходник](../src/serve.c)
