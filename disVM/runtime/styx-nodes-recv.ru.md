# Styxnodes.Node.recv

[English](styx-nodes-recv.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L39) · [Реализация](../limbo/styxnodes.b#L152)

```limbo
include "sys.m";
include "styxnodes.m";
recv: fn(node: self ref Node): ref Request;
```

Дождаться запроса в потоке-владельце Limbo. `node` — действующая
регистрация. Результат содержит точный `ticket`, строку операции
(`"r"`, `"w"` или `"x"`) и собственный массив до 65 536 байтов.
Буфер чтения заполнен нулями. При записи и исполнении массив содержит
входной фрагмент. Сохраняйте билет и операцию. В ожидании вызов
периодически передаёт управление планировщику Limbo. Закрытая или
недоступная регистрация даёт исключение.

```limbo
request := node.recv();
```

Завершите запрос через [reply](styx-nodes-reply.ru.md) или
[refuse](styx-nodes-refuse.ru.md).
