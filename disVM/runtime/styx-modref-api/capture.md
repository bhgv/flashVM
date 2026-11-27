# Styxmodref.capture

[Русский](capture.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
capture: fn(path: string, writable: int): ref Cell;
```

Pass the canonical relative path, for example `mounted/ref-pair.dis/.var/counter`, and `writable` equal to 0 or 1. The path is relative to the original attached FlashVM. The call opens that export, verifies the exact fid and returns an owned `ref Cell`. It can block while FlashVM serves the open and stat. Invalid path or mode, missing service, refused access or wrong file type raises an exception. Read using [Cell.read](read.md) or send the descriptor from [Cell.payload](payload.md).
