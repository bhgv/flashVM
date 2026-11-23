# styxrpc_launch_named_status

[English](launch_named_status.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/launch.h>
StyxRpcLaunchStatus styxrpc_launch_named_status(struct StyxFid *request);
```

Читает по тому же fid, который выдал launch_named, с байтового смещения ноль. PENDING означает ожидание результата. OK и FAILED подтверждают исход программы. UNKNOWN означает ошибку транспорта, неверный fid или повреждённый ответ. Повторное чтение не запускает программу.

Hosted Linux64. `StyxRpc::launch`.

[Пример](../examples/disvm/named.c) · [Исходник](../src/launch.c)
