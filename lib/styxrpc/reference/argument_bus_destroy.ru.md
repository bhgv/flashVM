# styxrpc_argument_bus_destroy

[English](argument_bus_destroy.md) · [Объявление](../include/styxrpc/channel_arguments.h#L31) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
void styxrpc_argument_bus_destroy(StyxRpcArgumentBus *bus);
```

Освободите собственную `bus` после остановки встречного сервера и закрытия всех операций и областей. Выполняйте уничтожение последовательно с управлением областями. Допускается `NULL`. Заимствованные описатели становятся недействительными.

[Проверка C](../tests/channel_arguments.c)
