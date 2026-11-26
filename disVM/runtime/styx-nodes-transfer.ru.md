# StyxnodeHandler.transfer

[English](styx-nodes-transfer.md) · [Руководство](STYX-NODES.ru.md) · [Интерфейс](../module/styxnodes.m#L8) · [Реализация](../../lib/styxrpc/tests/flashvm/nodes_handler.b)

```limbo
include "sys.m";
include "styxnodes.m";
transfer: fn(path, operation: string, buffer: array of byte): int;
```

Реализуйте функцию в модуле Limbo, передаваемом
[dispatch](styx-nodes-dispatch.ru.md). `path` — абсолютное имя,
выбранное при публикации. `operation` равна `"r"` при чтении,
`"w"` при записи и `"x"` при исполнении. `buffer` — изменяемый
массив байтов запроса. `len buffer` — ёмкость чтения или длина входа.
Для чтения и исполнения заполните префикс. Для записи подтвердите
обработанный префикс входа. Верните число байтов от нуля до
`len buffer` или отрицательное число для отказа. Исключение также
означает отказ. Модуль выполняется в потоке владельца. Завершите ответ
до синхронного вызова к ожидающему участнику.

См. [двоичный обработчик проверки](../../lib/styxrpc/tests/flashvm/nodes_handler.b)
с рабочим примером всех трёх операций.
