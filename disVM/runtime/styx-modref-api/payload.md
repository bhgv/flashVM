# Styxmodref.Cell.payload

[Русский](payload.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
payload: fn(cell: self ref Cell): array of byte;
```

Call `port.send(cell.payload())` to forward the original identity and access mode on the same pair. The call copies the complete frame, including the named-channel tag, into an owned byte array. It does not read the source value or open an adopted export. A closed or unavailable cell raises an exception. Serialize payload reads with other operations on this cell.
