# Styxmodref.Cell.close

[English](close.md) · [Объявление](../../module/styxmodref.m) · [Реализация](../../limbo/styxmodref.b) · [Руководство](../STYX-MODULE-REFS.ru.md)

`styxmodref.dis`

```limbo
close: fn(cell: self ref Cell);
```

Вызовите `cell.close()` после завершения операций с ячейкой. Он освобождает fid прокси и его удержание экспорта. Повторное закрытие той же ячейки безопасно. Участник канала и другие ссылки имеют независимый срок жизни. Порт C ставит открытый fid в очередь на освобождение без синхронного запроса Styx.
