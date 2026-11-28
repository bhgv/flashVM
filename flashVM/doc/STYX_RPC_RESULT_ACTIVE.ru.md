# endpoint.active

[English](STYX_RPC_RESULT_ACTIVE.md) · [Руководство](STYX_RPC_RESULT_CHANNELS.ru.md) · [Реализация](../src/styxrpc/channel_result_endpoint.c#L177)

## Синтаксис

```actionscript
endpoint.active():Boolean
```

Вызов без аргументов. Возвращает `true`, пока конец готов к публикации или выданный ресурс действует. Возвращает `false` после локального закрытия, отклонения публикации с поглощённым ресурсом или отзыва аренды партнёра. Снимает состояние под мьютексом очереди. Результат не резервирует операцию и может устареть к следующему вызову.

```actionscript
if (owner.active()) {
    last = owner.tryReceive();
}
```

[Закрытие](STYX_RPC_RESULT_CLOSE.ru.md) · [Проверка в C](../../lib/styxrpc/reference/channel_mailbox_active.ru.md)
