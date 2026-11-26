# text

[Русский](text.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxvalue.m#L71) · [Implementation](../../limbo/styxvalue.b#L97)

```limbo
text(value: string): ref Value
```

Copy a Limbo string into a Textual value carrying counted UTF-8 bytes. Returns an owned value, performs no I/O and accepts empty text. The send or encode step applies the frame size bound.

```limbo
value := codec->text("готово");
```
