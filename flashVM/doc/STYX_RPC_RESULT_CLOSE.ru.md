# endpoint.close

[English](STYX_RPC_RESULT_CLOSE.md) · [Руководство](STYX_RPC_RESULT_CHANNELS.ru.md) · [Реализация](../src/styxrpc/channel_result_endpoint.c#L194)

## Синтаксис

```actionscript
endpoint.close():Boolean
```

Вызов без аргументов. Освобождает локальную ссылку и однократно возвращает `true`. Повторное закрытие возвращает `false`. Ещё не выданный ресурс освобождается вместе с концом. Выданный ресурс доступен по аренде партнёра до освобождения fid функции. Закрытие выполняется локально без сетевого ожидания. Сборщик мусора освобождает такую же ссылку владельца.

```actionscript
var released = owner.close(); // Все локальные алиасы видят закрытое состояние.
```

[Активность](STYX_RPC_RESULT_ACTIVE.ru.md) · [Освобождение владельца C](../../lib/styxrpc/reference/channel_mailbox_owner_release.ru.md)
