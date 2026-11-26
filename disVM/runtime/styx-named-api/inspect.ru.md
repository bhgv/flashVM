# inspect

[English](inspect.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxvalue.m#L59) · [Реализация](../../limbo/styxvalue_graph_read.b#L219)

```limbo
inspect(bytes: array of byte): ref DataGraph
```

Читает переносимые собственные записи из FVMG v8/v9 без тега именованного сообщения. Возвращает граф с корнем и индексами узлов. Другой профиль даёт nil, повреждение даёт исключение. Байты заимствуются при разборе. Возвращённые данные правятся независимо. Колбэки VM не переносятся. Ввода-вывода нет.

```limbo
pick envelope := value { Graph => graph = codec->inspect(envelope.bytes); }
```
