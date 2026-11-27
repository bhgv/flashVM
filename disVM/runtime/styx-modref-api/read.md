# Styxmodref.Cell.read

[Русский](read.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
read: fn(cell: self ref Cell): int;
```

Call `cell.read()` on a live cell to retrieve the current signed Int32 as a Limbo `int`. The request can wait for the original issuer, including on the first access to an adopted cell. A closed cell, stale Qid, disconnected issuer or incomplete value raises an exception. Serialize this call with writes, payload reads and closure on the same cell.

In a FlashVM pair, `Processctl.cancelwait(pid)` can interrupt this wait in a direct child. A later read checks the original Qid if the remote fid was retired.
[Runnable Limbo and FlashVM example](../../../lib/styxrpc/examples/disvm/REF_PORT_CANCELLATION.md).
