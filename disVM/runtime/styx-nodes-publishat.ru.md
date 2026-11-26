# Styxnodes.publishat

[English](styx-nodes-publishat.md) · [Руководство](STYX-NODE-OFFSETS.ru.md) · [Объявление](../module/styxnodes.m#L68) · [Исходник](../limbo/styxnodes.b#L108)

```limbo
include "sys.m";
include "styxnodes.m";
publishat: fn(path, permissions: string, length: big): ref Node;
```

Опубликовать узел со смещениями в живом пространстве имён запуска.
Правила `path` и `permissions` совпадают с [publish](styx-nodes-publish.ru.md).
`length` — число байтов от нуля до 2⁵³−1. Длина видна в метаданных VFS
и сохраняется до отзыва. Узел владеет fid регистрации. Неверное
значение, занятый путь или отказ публикации дают исключение.
Обслуживайте узел в потоке-владельце.

```limbo
file := nodes->publishat("/tools/data", "rw", big 4096);
```

Для работы со смещениями вызывайте [dispatchat](styx-nodes-dispatchat.ru.md)
или [recvat](styx-nodes-recvat.ru.md). Отзыв через [close](styx-nodes-close.ru.md).
