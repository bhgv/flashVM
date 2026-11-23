# styxrpc_channel_invoke

[English](channel_invoke.md) · [Объявление](../include/styxrpc/channel_client.h#L26) · [Реализация](../src/channel_client.c) · [Описание аренды](channel_leases.ru.md)

Подключите `StyxRpc::launch`.

```c
#include <styxrpc/channel_client.h>
StyxRpcOutcome styxrpc_channel_invoke(const StyxRpcManagedTarget *target, const void *arguments,
    size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result);
```

Передайте смонтированную цель `target` с точным путём модуля, именем и подписью, полной схемой, ненулевым опубликованным PID и сроком опроса ответа 1–60000 мс. `arguments` содержит закодированные дочерние записи. Для пустого списка передайте `NULL, 0`. `reply` — отдельный буфер ёмкостью `capacity` от 1 до 65536 байт. Профиль аренды принимает значения по копии и типы каналов. Для ref и гибридных таблиц предназначен графовый API. Вызывающий владеет сессией, сериализует доступ и сохраняет встречную публикацию.

`STYXRPC_CALL_OK` записывает в `result` представление записи из `reply`, копию разных маршрутов и одну собственную ссылку на аренду при возврате канала. Освободите её через [lease release](channel_lease_release.ru.md). До записи функция сверяет контекстное предложение и оба маркера по одному fid модуля. `STYXRPC_CALL_NOT_SENT` означает подтверждённый отказ до отправки. `STYXRPC_CALL_REMOTE_ERROR` подтверждает удалённую ошибку. При `STYXRPC_CALL_UNCERTAIN` исход неизвестен. Функция отправляет один вызов. Для локальных очередей аргументов используйте [вызов с областью](argument_scope_invoke.ru.md).

[Исполняемая проверка](../examples/disvm/routed_client.c)
