# Cell.getvalue

[Русский](getvalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m#L63) · [Implementation](../../limbo/styxnamed.b#L300)

```limbo
Cell.getvalue(): ref Styxvalue->Value
```

Read the cell through its open fid and return the current value as owned typed data. Read can block awaiting the reply. A closed lease, malformed reply, or transport failure raises. Order operations on the same fid sequentially.

```limbo
current := cell.getvalue();
```
