# Styxmodref.Cell.close

[Русский](close.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
close: fn(cell: self ref Cell);
```

Call `cell.close()` once operations on the cell have finished. It releases the proxy fd and its hold on the export. Repeated close on the same cell is harmless. A channel participant and other references have independent lifetimes. The underlying C port queues its fid retirement without a synchronous Styx request.
