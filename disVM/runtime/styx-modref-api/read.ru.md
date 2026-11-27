# Styxmodref.Cell.read

[English](read.md) · [Объявление](../../module/styxmodref.m) · [Реализация](../../limbo/styxmodref.b) · [Руководство](../STYX-MODULE-REFS.ru.md)

`styxmodref.dis`

```limbo
read: fn(cell: self ref Cell): int;
```

Вызовите `cell.read()` для живой ячейки, чтобы получить текущее значение знакового Int32 как Limbo `int`. Запрос может ждать владельца, в том числе при первом чтении принятой ссылки. Закрытая ячейка, старый Qid, обрыв подключения или неполное значение вызывают исключение. Выполняйте чтение, запись, получение кадра и закрытие одной ячейки последовательно.

В паре с FlashVM `Processctl.cancelwait(pid)` может прервать ожидание при обращении прямого потомка. Следующее чтение сверяет исходный Qid, если удалённый fid был освобождён.
[Исполняемый пример Limbo и FlashVM](../../../lib/styxrpc/examples/disvm/REF_PORT_CANCELLATION.ru.md).
