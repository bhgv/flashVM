# Styxnodes.Node.refuse

[English](styx-nodes-refuse.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L47) · [Реализация](../limbo/styxnodes.b#L190)

```limbo
include "sys.m";
include "styxnodes.m";
refuse: fn(node: self ref Node, request: ref Request): int;
```

Отклонить точный ожидающий `request`, полученный через `node`.
Локальная запись заимствует запрос и отправляет его билет.
Функция возвращает единицу, если служба приняла отказ, и ноль при
закрытом узле, устаревшем билете или ошибке ввода-вывода.
Ожидающий вызов получает ошибку.

```limbo
accepted := node.refuse(request);
```

См. [recv](styx-nodes-recv.ru.md) и [reply](styx-nodes-reply.ru.md).
