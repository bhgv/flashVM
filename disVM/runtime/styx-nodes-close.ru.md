# Styxnodes.Node.close

[English](styx-nodes-close.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L55) · [Реализация](../limbo/styxnodes.b#L239)

```limbo
include "sys.m";
include "styxnodes.m";
close: fn(node: self ref Node);
```

Освободить fid регистрации, которым владеет `node`. Вызов ничего
не возвращает и допускает повтор. При освобождении последнего
владельца fid удаляется точный слой пространства имён. Ожидающий
вызов без ответа получает ошибку. Вызывайте в потоке-владельце.
Для отзыва после close не храните другую прямую ссылку на `node.fd`.
Отдельно открытые прикладные файлы не удерживают регистрацию.

```limbo
node.close();
```

См. [publish](styx-nodes-publish.ru.md) и
[парный пример](STYX-NODES.ru.md#проверка-в-паре).
