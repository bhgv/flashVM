# Styxnodes.Node.dispatch

[English](styx-nodes-dispatch.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L51) · [Реализация](../limbo/styxnodes.b#L197)

```limbo
include "sys.m";
include "styxnodes.m";
dispatch: fn(node: self ref Node, handler: StyxnodeHandler): int;
```

Дождаться одного запроса на `node` и синхронно вызвать заимствованный
модуль `handler` в текущем потоке Limbo. Возвращает единицу, когда служба
владельца приняла ответ или отказ, и ноль, если доставить ответ не удалось.
Исключение колбэка или неверное число байтов превращаются в отказ.
Отсутствующий модуль или ошибка приёма дают исключение приложению.
Продолжайте обслуживание повторным вызовом в том же потоке.

```limbo
if (!node.dispatch(handler)) {
    sys->print("reply expired\n");
}
```

См. [transfer](styx-nodes-transfer.ru.md) и [recv](styx-nodes-recv.ru.md).
