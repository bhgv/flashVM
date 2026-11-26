# Port.sendvalue

[Русский](sendvalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m#L41) · [Implementation](../../limbo/styxnamed.b#L273)

```limbo
Port.sendvalue(message: ref Message)
```

Send one typed message on the open port. Data is encoded before any transport write. Shared borrows a live Cell lease until a receiver accepts it. Intrefdata borrows the exact validated legacy descriptor. Success returns after rendezvous. Invalid data or I/O failure raises an exception. Serialize calls on a port.

```limbo
port.sendvalue(ref Message.Data(codec->text("ready")));
```
