# build

[English](build.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxvalue.m#L63) · [Реализация](../../limbo/styxvalue_graph_write.b#L280)

```limbo
build(graph: ref DataGraph): ref Value
```

Кодирует достижимые записи заимствованного DataGraph как собственное Value.Graph в FVMG v8. Имена полей и числовые ключи должны быть уникальны. Ссылки используют индексы массива nodes с нуля. Циклы и общие узлы поддерживаются. Нет цели, неверный профиль или снимок длиннее 65 535 байт без тега дают исключение. Ввода-вывода нет.

```limbo
port.sendvalue(ref Message.Data(codec->build(graph)));
```
