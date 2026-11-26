# Styxnodes.init

[English](styx-nodes-init.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L53) · [Реализация](../limbo/styxnodes.b#L15)

```limbo
include "sys.m";
include "styxnodes.m";
init: fn();
```

Загрузить системный модуль `Sys`. Вызовите после загрузки `Styxnodes`,
до публикации или обслуживания узлов. Аргументов и возвращаемого
значения нет. Регистрация при этом не открывается. Повторный вызов
после успешной загрузки сохраняет действующие публикации.

```limbo
nodes := load Styxnodes Styxnodes->PATH;
if (nodes != nil) {
    nodes->init();
}
```

См. [publish](styx-nodes-publish.ru.md).
