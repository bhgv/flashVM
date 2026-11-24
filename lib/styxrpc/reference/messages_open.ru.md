# styxrpc_messages_open

[English](messages_open.md) · [Заголовок](../include/styxrpc/messages.h) · [Реализация](../src/message_client.c)

Подключите `styxrpc/messages.h` и библиотеку `StyxRpc::launch`.

```c
StyxRpcStatus styxrpc_messages_open(struct StyxClient *session, const StyxRpcMessageSpec *spec,
    StyxRpcMessagePort **out);
```

`session` — подключённый клиент с файлом `.messages` в корне. В `spec`
передайте от одной до 32 регистраций. SDK копирует имена и описатели.
Сохраняйте контексты до закрытия. На клиенте используются только `types`
и `count`. Для каждой схемы требуются ненулевая версия, имя из 1–63
непробельных печатных ASCII-символов и тег отправки, обработчик или оба
вместе. Пары имени и версии и ненулевые теги не могут повторяться.

Вызов синхронно открывает fid на чтение и запись со сроками ожидания Styx.
При `STYXRPC_OK` в `out` записывается порт, принадлежащий клиенту.
При `STYXRPC_INVALID` значение `out` сохраняется. Реестр фиксируется
до закрытия. Сохраняйте `session` и её владельца на время работы порта.
Обмен объявлениями и доставка выполняются через
[styxrpc_messages_pump](messages_pump.ru.md).

[Настройка обмена сообщениями](custom_messages.ru.md) · [Проверка на сокете](../tests/message_socket.c)
