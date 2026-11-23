# styxrpc_argument_bus_create

[English](argument_bus_create.md) · [Объявление](../include/styxrpc/channel_arguments.h#L21) · [Реализация](../src/channel_arguments.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcArgumentBus *styxrpc_argument_bus_create(uint32_t pid);
```

Выделите собственную встречную шину аргументов для ненулевого `pid`. Функция возвращает дескриптор или `NULL` при неверном PID, нехватке памяти или невозможности создать блокировку. Удерживайте одну шину на выбранном встречном соединении. Опубликуйте её [ресурс](argument_bus_resource.ru.md) до запуска диспетчера и [уничтожьте](argument_bus_destroy.ru.md) после остановки.

[Проверка C](../tests/channel_arguments.c)
