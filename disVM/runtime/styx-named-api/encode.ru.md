# encode

[English](encode.md) · [Руководство](../STYX-NAMED-VALUES.ru.md) · [Объявление](../../module/styxvalue.m#L75) · [Реализация](../../limbo/styxvalue.b#L104)

```limbo
encode(value: ref Value): array of byte
```

Кодирует заимствованное значение с байтом типа в собственный кадр до 65 536 байт. Проверяет версию, длину и контрольную сумму графа. Пустой указатель, неверное значение, внутренняя ссылка графа и превышение длины дают исключение. Ввода-вывода нет. Байты Textual в UTF-8/WTF-8 копируются как переданы.

```limbo
frame := codec->encode(ref Value.Numeric(-1.25));
```
