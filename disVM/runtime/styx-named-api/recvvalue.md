# Port.recvvalue

[Русский](recvvalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m) · [Implementation](../../limbo/styxnamed.b)

```limbo
Port.recvvalue(): ref Message
```

Wait for the next message. Data contains an owned typed value. Shared contains a newly opened cell fid acquired before return. Intrefdata preserves the descriptor bytes for an explicitly mounted issuer. Modref contains an adopted [module reference](../STYX-MODULE-REFS.md) in `reference`. Its exact export is opened during the first value operation. Close owned cells and references when finished. Invalid frames or I/O failure raise an exception. Serialize calls on the same port.

```limbo
message := port.recvvalue();
```
