# Styxnodes.Node.reply

[English](styx-nodes-reply.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L43) · [Реализация](../limbo/styxnodes.b#L176)

```limbo
include "sys.m";
include "styxnodes.m";
reply: fn(node: self ref Node, request: ref Request, count: int): int;
```

Завершить принятый запрос на узле-владельце. `count` — число байтов
от нуля до исходной длины буфера. Для чтения и исполнения передаётся
префикс `request.buffer`. При записи подтверждается это число входных
байтов. Вызов заимствует запрос на время локальной записи. Возвращает
единицу, если ответ принят, и ноль при неверном числе, закрытом узле,
устаревшем билете или ошибке ввода-вывода. Повторный ответ сохраняет
ранее принятый результат.

```limbo
request.buffer[0] = byte 'A';
accepted := node.reply(request, 1);
```

Здесь предполагается непустой буфер чтения от [recv](styx-nodes-recv.ru.md).
При ошибке обработки используйте [refuse](styx-nodes-refuse.ru.md).
