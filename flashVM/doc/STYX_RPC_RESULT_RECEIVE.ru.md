# endpoint.tryReceive

[English](STYX_RPC_RESULT_RECEIVE.md) · [Руководство](STYX_RPC_RESULT_CHANNELS.ru.md) · [Реализация](../src/styxrpc/channel_result_endpoint.c#L115)

## Синтаксис

```actionscript
endpoint.tryReceive():Object
```

Вызов без аргументов. Возвращает копию типизированного значения партнёра либо `undefined` при пустом входящем FIFO или отсутствии ожидающего отправителя rendezvous. Строка nil возвращается как `null`. Закрытый конец вызывает исключение. После извлечения кадр декодируется у владельца AS, где возможен сбой памяти или преобразования. Автоматического повтора нет. Вызывайте между кадрами или тактами владельца без ожидания сетевого запроса.

```actionscript
var message = owner.tryReceive();
if (message !== undefined) {
    latest = message;              // Строка nil считается принятым значением.
}
```

[Отправка](STYX_RPC_RESULT_SEND.ru.md) · [Типы](STYX_RPC_VALUE_TYPES.ru.md)
