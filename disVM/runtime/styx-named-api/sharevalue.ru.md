# sharevalue

[English](sharevalue.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxnamed.m#L88) · [Реализация](../../limbo/styxnamed.b#L316)

```limbo
sharevalue(value: ref Styxvalue->Value): ref Cell
```

Кодирует заимствованное значение и публикует ячейку в штатной парной сессии. Возвращает первую собственную аренду. Неверные данные отклоняются до публикации. Сбой транспорта даёт исключение. Сохраняйте аренду при выдаче дескриптора и освобождайте через close.

```limbo
cell := named->sharevalue(ref Value.Numeric(7.5));
```
