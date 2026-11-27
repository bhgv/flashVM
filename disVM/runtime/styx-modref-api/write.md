# Styxmodref.Cell.write

[Русский](write.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
write: fn(cell: self ref Cell, value: int);
```

Call `cell.write(42)` on a live writable cell to replace the original Int32 with `value`. The operation uses the retained fid or checks the adopted descriptor before its first open. It may block for the issuer. A read-only or closed cell, retired export or failed write raises an exception. A transport error can leave the update uncertain. Serialize operations on this cell and keep other aliases alive as needed.
