# StyxnodePositionedHandler.transfer

[English](styx-nodes-transfer-at.md) · [Руководство](STYX-NODE-OFFSETS.ru.md) · [Объявление](../module/styxnodes.m#L15) · [Исходник](../../lib/styxrpc/tests/flashvm/positioned_handler.b)

```limbo
include "sys.m";
include "styxnodes.m";
transfer: fn(path, operation: string, buffer: array of byte, offset: big): int;
```

Реализуйте функцию в модуле, который передаётся в
[dispatchat](styx-nodes-dispatchat.ru.md). `path` — абсолютное имя
регистрации. `operation` равна `"r"`, `"w"` или `"x"`. `buffer` —
изменяемый массив байтов. Читайте и записывайте с абсолютного
смещения `offset` в байтах. `len buffer` — ёмкость чтения или объём
входа для записи и исполнения. Для исполнения смещение всегда ноль.
Верните подтверждённое число от нуля до `len buffer`. При чтении и
исполнении заполните этот префикс. Отрицательное число или исключение
означают отказ. Колбэк работает в потоке-владельце Limbo.

См. [обработчик разреженного файла](../../lib/styxrpc/tests/flashvm/positioned_handler.b).
