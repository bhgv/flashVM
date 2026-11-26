# Node.reply

[English](styx-exec-reply.md) · [Руководство](STYX-EXEC-PUBLISH.ru.md) · [Интерфейс](../module/styxexec.m#L33) · [Реализация](../limbo/styxexec.b#L161)

```limbo
node.reply(request: ref Request, bytes: array of byte): int
```

Передайте байты для Request, полученного на этом Node. Длина ответа может быть нулевой и ограничена длиной входа. Хост копирует байты. Результат 1 означает подтверждение, 0 означает неверные аргументы, истечение срока, повторный ответ либо сбой локальной записи.

```limbo
accepted := node.reply(item, item.bytes);
```
