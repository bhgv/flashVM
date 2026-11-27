# Vfsexport.bigvar

[Русский](VFS-BIG-VARIABLE.ru.md) · [Limbo interface](../module/vfsexport.m#L44) · [Implementation](vfs/cap_live_value.c#L274) · [Paired example](../../lib/styxrpc/examples/disvm/LIVE_STAGE.md)

## Syntax

```limbo
include "vfsexport.m";
bigvar: fn(initial: big): ref Root;
```

Load `Vfsexport` from `Vfsexport->PATH` on the native DisVM runtime. Pass any signed Limbo `big` value as `initial`. Returns an owned `Root` that retains the value, or `nil` if native storage or the file projection cannot be allocated. The cell covers the full signed 64-bit interval.

## Files and lifetime

| File | Contract |
| --- | --- |
| `.type` | Read-only `big
`. |
| `.val` | One complete scalar `big` child record, tag 2. Use [the SDK integer format](../../lib/styxrpc/reference/value_integer.md). |

The first nonempty read request snapshots the value for the open fid. Give each new sample its own read fid. The cell accepts one complete signed-value record written at offset zero. Wrong tag, nonvalue record, incomplete transfer or invalid offset is refused. A reader sees either the preceding or the replacing value. The published provider and open fids retain native storage after mounting. Use [record composition](VFS-VARIABLE-RECORD.md) to publish it with other fields.

## Example

```limbo
# Retain the full-width initial value for a remote client.
caps := load Vfsexport Vfsexport->PATH;
magnitude := caps->bigvar(big "-9223372036854775808");
```

Check `magnitude` before mounting it or adding it as a field. The C client reads an `int64_t` from [the live read API](../../lib/styxrpc/reference/live_read_until.md). FlashVM reads that minimum as decimal text from [`binding.get()`](../../flashVM/doc/STYX_RPC_LIVE_GET.md). Safe integers decode as AS Number.

[Big vector](VFS-BIG-VECTOR.md) · [String cell](VFS-STRING-VARIABLE.md)
