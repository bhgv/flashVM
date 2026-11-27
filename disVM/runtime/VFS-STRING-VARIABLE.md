# Vfsexport.stringvar

[Русский](VFS-STRING-VARIABLE.ru.md) · [Limbo interface](../module/vfsexport.m#L41) · [Implementation](vfs/cap_live_value.c#L255) · [Paired example](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md)

## Syntax

```limbo
include "vfsexport.m";
stringvar: fn(initial: string): ref Root;
```

Load `Vfsexport` from `Vfsexport->PATH` on the native DisVM runtime. Pass the initial string or `nil`. The initial value is copied to native storage. Returns an owned `Root` for a live string capability. Returns `nil` on allocation failure, invalid encoding or a string above 1,024 encoded UTF-8 bytes. Empty and nil strings have different encodings. A counted UTF-8 string can contain zero.

## Files and lifetime

| File | Contract |
| --- | --- |
| `.type` | Read-only `string
`. |
| `.val` | One complete UniBus string child (tag 7 and counted UTF-8) or nil child (tag 9 and empty payload). Read using a fresh fid to acquire a new snapshot. Write one complete record at byte offset zero. |

An open reader takes its snapshot on the first nonempty read request. Subsequent fragments on the same open recover that snapshot. A validated whole write replaces the native value under the cell lock. Partial, invalid or nonzero-offset writes fail. Keep the capability rooted while composing or mounting it. The published provider and open fids then retain the native storage while in use. An unavailable or unauthorized path fails independently of cell creation.

## Example

```limbo
# Install this root as a field of a record before exporting the VFS.
caps := load Vfsexport Vfsexport->PATH;
label := caps->stringvar("sun");
r := caps->record(("label", label) :: nil);
```

Check the constructor results before mounting `r`. Open `label` through the remote [FlashVM live binding](../../flashVM/doc/STYX_RPC_LIVE_GET.md) or the [hosted C SDK](../../lib/styxrpc/reference/live_read_until.md). Read a fresh value from Limbo by opening `.val` with `Vfsexport.open` and decoding the counted record.

[Record publication](VFS-VARIABLE-RECORD.md) · [Big cell](VFS-BIG-VARIABLE.md) · [Live projection format](../../lib/styxrpc/LIVE_VALUES.md)
