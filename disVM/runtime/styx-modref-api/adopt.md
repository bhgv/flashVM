# Styxmodref.adopt

[Русский](adopt.ru.md) · [Declaration](../../module/styxmodref.m) · [Implementation](../../limbo/styxmodref.b) · [Guide](../STYX-MODULE-REFS.md)

`styxmodref.dis`

```limbo
adopt: fn(frame: array of byte): ref Cell;
```

Pass the complete tagged frame received from the named channel, as in `refs->adopt(pipe.recv())`. The call copies the descriptor, checks its format, pairing scope and issuer role, and returns an owned `ref Cell`. It accepts exports from both VMs on a current pair. An invalid or unsupported frame raises an exception. The exact export opens on the first value read or write. When DisVM owns the export, that access checks the path and Qid issued by its reverse server. Keep the owner available at that point. [Close](close.md) the returned cell in the usual way.
