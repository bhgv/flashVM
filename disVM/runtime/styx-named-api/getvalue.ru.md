# Cell.getvalue

[English](getvalue.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxnamed.m#L63) · [Реализация](../../limbo/styxnamed.b#L300)

```limbo
Cell.getvalue(): ref Styxvalue->Value
```

Читает ячейку через её открытый fid и возвращает текущее значение во владение вызывающему. Чтение может ждать ответа. Закрытая аренда, неверный ответ или сбой транспорта дают исключение. Операции на fid выполняются последовательно.

```limbo
current := cell.getvalue();
```
