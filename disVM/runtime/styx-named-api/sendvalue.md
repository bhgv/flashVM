# Port.sendvalue

[Русский](sendvalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m) · [Implementation](../../limbo/styxnamed.b)

```limbo
Port.sendvalue(message: ref Message)
```

Send one typed message on the open port. Data is encoded before any transport write. Shared borrows a live Cell lease until a receiver accepts it. Intrefdata borrows the exact validated legacy descriptor. Modref forwards the current [module reference](../STYX-MODULE-REFS.md) with its original scope and access mode. Keep the reference open during the call. Success returns after rendezvous. Invalid data or I/O failure raises an exception. Serialize calls on a port.

```limbo
port.sendvalue(ref Message.Data(codec->text("ready")));
```
