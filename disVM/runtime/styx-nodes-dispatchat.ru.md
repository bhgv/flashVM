# Styxnodes.dispatchat

[English](styx-nodes-dispatchat.md) · [Руководство](STYX-NODE-OFFSETS.ru.md) · [Объявление](../module/styxnodes.m#L76) · [Исходник](../limbo/styxnodes.b#L218)

```limbo
include "sys.m";
include "styxnodes.m";
dispatchat: fn(node: ref Node, handler: StyxnodePositionedHandler): int;
```

Принять один запрос на действующем `node` и вызвать заимствованный
модуль `handler` в том же потоке Limbo. Обработчик получает путь,
операцию, буфер и абсолютное смещение в байтах. Результат равен
единице, если локальная очередь приняла ответ или отказ, и нулю,
если ответ истёк или доставка не удалась. Исключение или неверное
число байтов от обработчика означают отказ. Отсутствующий обработчик
и ошибка приёма дают исключение приложению.

```limbo
accepted := nodes->dispatchat(file, handler);
```

См. [transfer](styx-nodes-transfer-at.ru.md) и [recvat](styx-nodes-recvat.ru.md).
