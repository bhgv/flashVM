# sharevalue

[Русский](sharevalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m) · [Implementation](../../limbo/styxnamed.b)

```limbo
sharevalue(value: ref Styxvalue->Value): ref Cell
```

Encode the borrowed value and publish a named cell on the default paired session. Return its first owned lease. Invalid data fails before publication. A transport failure raises. Keep the lease while exporting its descriptor and release it with close.

```limbo
cell := named->sharevalue(ref Value.Numeric(7.5));
```
