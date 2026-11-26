# decode

[Русский](decode.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxvalue.m#L79) · [Implementation](../../limbo/styxvalue.b#L141)

```limbo
decode(frame: array of byte): ref Value
```

Decode a borrowed 1 to 65,536 byte tagged data frame. Return a typed owned value. Text is copied with embedded zeros and graph bytes are copied after envelope validation. Invalid lengths, tags, Boolean values and reference frames raise. No transport I/O occurs.

```limbo
value := codec->decode(frame);
```
