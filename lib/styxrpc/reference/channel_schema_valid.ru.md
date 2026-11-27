# styxrpc_channel_schema_valid

[English](channel_schema_valid.md) · [Объявление](../include/styxrpc/channel_call.h#L33) · [Реализация](../src/channel_call.c#L30) · [Руководство](../LIVE_VALUES.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_call.h>
int styxrpc_channel_schema_valid(const StyxRpcSchema *schema);
```

Подключите `StyxRpc::rpc`.

## Параметр и результат

Заимствует полную [`StyxRpcSchema`](../include/styxrpc/managed.h#L32) со списками типов аргументов и результата в скобках. Возвращает 1 для допустимой контекстной схемы хотя бы с одной позицией канала. Нулевая или несовместимая схема даёт 0. Ограниченная грамматика допускает копируемые значения, графовые ref и гибридную таблицу в одном вызове с каналами. Канал не может быть полем копируемого ref. Сообщение канала не может содержать ref или гибридную таблицу. Для списка по значению действуют его правила типов элементов.

Локальная проверка не зависит от потока и не создаёт fid или маршрутов. Для публикации скомпилированной сигнатуры используйте [`styxrpc_channel_profile`](channel_profile.ru.md). Для проверки предложения и вызова служит [`styxrpc_channel_invoke`](channel_invoke.ru.md).

## Пример

```c
const StyxRpcSchema schema = { "(r[b]c[i])", "((r[b]c[b]))" };
if (styxrpc_channel_schema_valid(&schema)) {
    /* Провайдер может вернуть изменённый ref и собственный канал. */
}
```
