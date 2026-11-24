# styxrpc_message_send

[English](message_send.md) · [Заголовок](../include/styxrpc/messages.h) · [Реализация](../src/message_port.c)

Подключите `styxrpc/messages.h` и библиотеку `StyxRpc::rpc`.

```c
StyxRpcStatus styxrpc_message_send(StyxRpcMessagePort *port, const char *name, uint32_t version,
    const void *payload, size_t size);
```

Передайте действующий `port`, точное зарегистрированное имя `name` и
версию `version` с ненулевым тегом отправки и `size` байт в `payload`.
Размер — от 0 до 16 384 байт, NULL допустим при нулевом размере.
`STYXRPC_OK` означает, что порт скопировал байты в очередь. При
`STYXRPC_MORE` очередь заполнена, и сообщение в неё не попало.
`STYXRPC_INVALID` означает ошибочный аргумент, отсутствие регистрации
для отправки или отказ порта.

Вызов выполняется владельцем порта, в том числе из колбэка приёма или
`opened`. Сетевых операций в нём нет. На клиенте передачу выполняет
[styxrpc_messages_pump](messages_pump.ru.md). Сервер выдаёт байты при чтении
fid. Если отправителю нужно подтверждение доставки и обработки,
предусмотрите отдельное ответное сообщение.

[Настройка обмена сообщениями](custom_messages.ru.md) · [Проверка на сокете](../tests/message_socket.c)
