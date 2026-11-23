# styxrpc_argument_scope_invoke

[English](argument_scope_invoke.md) · [Объявление](../include/styxrpc/channel_arguments.h#L57) · [Реализация](../src/channel_client.c) · [Сценарий](channel_argument_scopes.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_arguments.h>
StyxRpcOutcome styxrpc_argument_scope_invoke(StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result);
```

Поглотите собственную область подготовки `scope` при любом исходе. Передайте точную `target`, дочерние записи аргументов `arguments` длиной `size` байт, буфер `reply` ёмкостью `capacity` и `result`, как в [channel invoke](channel_invoke.ru.md). PID цели, проверенные ключи и типы аргументов должны соответствовать области. SDK сверяет предложение на смонтированном модуле и активирует ключи непосредственно перед отправкой. Встречную шину этого же соединения обслуживайте параллельно прямому вызову. Записи результата заимствуют буфер ответа. Успешно возвращённые каналы имеют отдельную аренду. Область отозвана к возврату. Неопределённый исход может включать уже завершённые операции с очередями.

[Проверка C](../tests/channel_arguments.c)
