# endpoint.trySend

[English](STYX_RPC_RESULT_SEND.md) · [Руководство](STYX_RPC_RESULT_CHANNELS.ru.md) · [Реализация](../src/styxrpc/channel_result_endpoint.c#L68)

## Синтаксис

```actionscript
endpoint.trySend(value):Boolean
```

Передайте одно значение [типа элемента](STYX_RPC_RESULT_CHANNEL.ru.md). Кодек копирует его по скомпилированной схеме в контексте владельца. Результат `true` подтверждает помещение в буфер или встречу с ожидающим получателем. При полном исходящем буфере или отсутствии получателя rendezvous возвращается `false`. В этих случаях значение остаётся у вызывающего. Закрытие конца и несовместимое значение приводят к исключению. Сетевого ожидания нет. Если удалённая сторона закроет аренду после успешной записи в буфер, непрочитанное значение может быть отброшено.

```actionscript
var sent = owner.trySend("ready"); // Для конца с типом "s".
if (!sent) {
    pendingText = "ready";          // Сохранить значение до следующего такта.
}
```

[Приём](STYX_RPC_RESULT_RECEIVE.ru.md) · [Пример](../../lib/styxrpc/examples/disvm/CHANNEL_RESULTS.ru.md)
