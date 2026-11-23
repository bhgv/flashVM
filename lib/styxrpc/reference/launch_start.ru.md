# styxrpc_launch_start

[English](launch_start.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_start(struct StyxClient *client, const void *arguments, size_t size,
    void *scratch, size_t capacity);
```

Заимствует подключённый client, кодирует size байт в отдельный scratch. Читает профиль raw и выполняет одну запись в .unibus. PENDING подтверждает точную доставку. UNKNOWN означает, что запуск мог начаться.

Linux64. Операции могут ждать до тайм-аута клиента Styx. `StyxRpc::launch`.

[Пример](../examples/disvm/launch.c) · [Исходник](../src/launch.c)
