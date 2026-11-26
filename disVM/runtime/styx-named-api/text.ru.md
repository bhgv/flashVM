# text

[English](text.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxvalue.m#L71) · [Реализация](../../limbo/styxvalue.b#L97)

```limbo
text(value: string): ref Value
```

Копирует строку Limbo в значение Textual с байтами UTF-8 и длиной. Возвращает собственное значение, выполняется без ввода-вывода и допускает пустую строку. Предел размера проверяется при кодировании или отправке.

```limbo
value := codec->text("готово");
```
