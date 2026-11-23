# styxrpc_channel_read_routed

[English](channel_read_routed.md) · [libstyxrpc](../README.ru.md)

## Синтаксис

```c
#include <styxrpc/channel_route.h>
StyxRpcStatus styxrpc_channel_read_routed(const StyxRpcChannelGrant *grants, size_t grant_count,
    const StyxRpcChannelPolicy *policy, const void *bytes, size_t size, StyxRpcSelection *operation);
```

Разберите полный запрос канала в буфере `bytes` размером `size` байтов.
Каждая ветка использует точный ключ из заимствованной таблицы `grants` на
`grant_count` записей. Необязательная `policy` проверяет каждый различный
адрес канала внутри типизированного значения. Нулевой указатель сохраняет
обычный разбор в пределах одного вызова. При заданной политике проверяются
все вложенные адреса, включая CURRENT.

Колбэк `allow` возвращает 1, чтобы разрешить точный адрес, ключ, ёмкость и тип
в текущей области полномочий. Колбэк выполняется без побочных эффектов.
Кодек может запросить решение снова при планировании и проверке ответа.
Укажите ту же политику в `StyxRpcChannelFid.policy` для адаптера fid или в
`StyxRpcSelection.policy` для запросов и ответов, подготовленных вручную.
Приложение сериализует колбэки и удерживает политику, контекст и буфер запроса
до завершения или clunk.

`STYXRPC_OK` выдаёт выбор, который заимствует буфер веток и политику.
`STYXRPC_INVALID` сохраняет `operation`. Неверные записи, неизвестные ключи и
отклонённые вложенные адреса вызывают отказ. Функция синхронная и без
транспортного ввода-вывода. Подключите `StyxRpc::rpc`.

[styxrpc_channel_fid_write](channel_fid_write.ru.md) · [Пример](../examples/disvm/routed_client.c) · [Исходник](../src/channel.c)
