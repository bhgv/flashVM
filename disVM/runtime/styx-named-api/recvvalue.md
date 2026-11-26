# Port.recvvalue

[Русский](recvvalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m#L37) · [Implementation](../../limbo/styxnamed.b#L254)

```limbo
Port.recvvalue(): ref Message
```

Wait for the next message. Data contains an owned typed value. Shared contains a newly opened cell fid acquired before return. Close that lease when finished. Intrefdata preserves the descriptor bytes for an explicitly mounted issuer. Invalid frames or I/O failure raise. Serialize calls on the same port.

```limbo
message := port.recvvalue();
```
