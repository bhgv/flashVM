# styxrpc_argument_bus_resource

[English](argument_bus_resource.md) · [Объявление](../include/styxrpc/channel_arguments.h#L26) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcResource styxrpc_argument_bus_resource(StyxRpcArgumentBus *bus);
```

Получите описатель, колбэки которого заимствуют `bus`. До запуска сервера опубликуйте его на [встречном .unibus](program_unibus.ru.md) указанного PID. Диспетчер вызывает колбэки независимо от прямого владельца. Открытие требует чтения и записи и активной области. Чтение ответа начинается со смещения подтверждённого запроса. Удерживайте шину до последнего закрытия ресурса.

[Проверка C](../tests/channel_arguments.c)
