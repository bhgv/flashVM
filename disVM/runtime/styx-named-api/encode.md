# encode

[Русский](encode.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxvalue.m#L75) · [Implementation](../../limbo/styxvalue.b#L104)

```limbo
encode(value: ref Value): array of byte
```

Encode a borrowed data value with its type byte into an owned frame of at most 65,536 bytes. Graph envelopes require correct version, length and checksum. Missing or invalid values, graph links and excessive lengths raise. No I/O occurs. Textual bytes must use UTF-8/WTF-8 and are copied as supplied.

```limbo
frame := codec->encode(ref Value.Numeric(-1.25));
```
