# styxrpc_channel_group_reverse_output

[English](channel_group_reverse_output.md) · [Объявление](../include/styxrpc/channel_group.h) · [Реализация](../src/channel_group.c) · [Транспорт](../src/channel_group_wire.c)

Подключите `<styxrpc/channel_group.h>` и линкуйте `StyxRpc::serve`.

```c
StyxRpcStatus styxrpc_channel_group_reverse_output(const StyxRpcChannelQueue *queues,
    size_t count, size_t root, StyxRpcChannelOutput *output);
```

Параметры совпадают с [channel_group_output](channel_group_output.ru.md). Передайте от одной до восьми очередей, индекс корневой очереди от нуля и выходную память. При успехе выход владеет копией группы, содержит ключ корня, признак ёмкости и колбэк освобождения. Неверный вход, неверные вложенные кадры и нехватка памяти дают `STYXRPC_INVALID`, сохраняя выход.

Зарегистрируйте `output->resource` через [styxrpc_program_unibus_grant](program_unibus_grant.ru.md) на разрешённом встречном диспетчере. Передайте выданный PID, ключ и тип корня в значении `STYXRPC_ROUTE_PID`. Чтение начинается после подтверждённой длины запроса. Начальный токен модуля не передаётся. Сериализуйте ресурс у одного диспетчера. [Отзовите публикацию](program_unibus_revoke.ru.md) либо завершите сервер с закрытием всех fid до вызова освобождения. Срок публикации контролирует хост C.

[Самостоятельная проверка](../tests/channel_group.c)
