# styxrpc_messages_close

[English](messages_close.md) · [Заголовок](../include/styxrpc/messages.h) · [Реализация](../src/message_client.c)

Подключите `styxrpc/messages.h` и библиотеку `StyxRpc::launch`.

```c
void styxrpc_messages_close(StyxRpcMessagePort *port);
```

Передайте принадлежащий клиенту `port`. Функция освобождает его,
отбрасывает очередь и закрывает fid через Styx clunk с обычным сроком
ожидания. Закройте порт перед выключением клиента. NULL допустим.
Повторно использовать освобождённый порт нельзя.

Вызывайте в потоке владельца вне колбэков этой сессии. Серверные порты
заимствуются у диспетчера. Он освобождает их при clunk или обрыве
связи, вызывая `closed` по одному разу на порт, если задан хук.

[Настройка обмена сообщениями](custom_messages.ru.md) · [Проверка на сокете](../tests/message_socket.c)

Для закрытия с отложенным I/O используйте [styxrpc_messages_abandon](messages_abandon.ru.md).
