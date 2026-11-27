# Styxmodref.capturelocal

[Русский](capturelocal.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
capturelocal: fn(path: string, writable: int): ref Cell;
```

Pass a canonical relative path such as `mounted/counter.dis/.var/value`.
The path starts at the DisVM root exported to the paired FlashVM.
Select mode `0` for reads or `1` for reads and writes. The current
publication must be a regular four-byte Int32 export that permits the
requested access.

The call opens the exact export and returns an owned `ref Cell`.
Its descriptor uses the same Qid as a walk from the peer. The call can
wait for local provider operations and raises on invalid input, a missing
paired service, refused access, or a changed export during open.
Forward it with [Port.sendvalue](../styx-named-api/sendvalue.md)
and `ref Message.Modref(cell)`. Serialize operations on the cell and
[close](close.md) it when finished. Use [capture](capture.md) for
exports owned by FlashVM.
