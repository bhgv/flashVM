# Cell.setvalue

[Русский](setvalue.ru.md) · [Guide](../STYX-NAMED-VALUES.md) · [Declaration](../../module/styxnamed.m) · [Implementation](../../limbo/styxnamed.b)

```limbo
Cell.setvalue(value: ref Styxvalue->Value)
```

Encode the borrowed value before writing. The cell replaces its complete value on confirmation and can change type. Invalid input raises before I/O. A transport error after submission can leave the result uncertain. Inspect the value before retrying a write. Serialize operations on this fid.

```limbo
cell.setvalue(ref Value.Logical(1));
```
