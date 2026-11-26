# Styxnodes.recvat

[English](styx-nodes-recvat.md) · [Руководство](STYX-NODE-OFFSETS.ru.md) · [Объявление](../module/styxnodes.m#L72) · [Исходник](../limbo/styxnodes.b#L118)

```limbo
include "sys.m";
include "styxnodes.m";
recvat: fn(node: ref Node): (ref Request, big);
```

Дождаться запроса на действующем `node` в потоке-владельце.
Возвращает тот же собственный запрос, что [recv](styx-nodes-recv.ru.md),
и его абсолютное смещение в байтах. Для исполнения и исходного
последовательного режима смещение равно нулю. Ошибки регистрации
и закрытый узел обрабатываются как в `recv`. Используются прежний
билет запроса и прежние операции ответа и отказа.

```limbo
(request, offset) := nodes->recvat(file);
```

Ответьте через [Node.reply](styx-nodes-reply.ru.md) или
[Node.refuse](styx-nodes-refuse.ru.md).
