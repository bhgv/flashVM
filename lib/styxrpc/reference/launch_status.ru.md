# styxrpc_launch_status

[English](launch_status.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_status(struct StyxClient *client);
```

Читает .exit после подтверждённой доставки. Возвращает PENDING до результата, OK или FAILED при ответе, UNKNOWN при обрыве или неверных данных. Автоматического повтора нет.

Linux64. Операции могут ждать до тайм-аута клиента Styx. `StyxRpc::launch`.

[Пример](../examples/disvm/launch.c) · [Исходник](../src/launch.c)
