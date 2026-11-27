# Typed Limbo values on a named StyxRPC channel

[Русский](STYX-NAMED-VALUES.ru.md) · [Transport declarations](../module/styxnamed.m) · [Value declarations](../module/styxvalue.m) · [Wire protocol](../../lib/styxrpc/reference/named_channel.md)

A channel in the paired namespace carries different types in successive
messages. In an application, include `sys.m` and `styxnamed.m`. The latter
includes the value definitions. Load `styxnamed.dis`, call its `init()`, then
open a name such as `/jobs` with `open()`. The typed methods use the installed
`styxvalue.dis` and DisVM's Math module. A program using the codec directly
loads `Styxvalue` and calls its own `init()`.

`Message.Data` carries `Value.Numeric` (binary64), `Textual` (counted
UTF-8/WTF-8 bytes), `Logical`, `Nullval`, `Undefinedval`, or `Graph`. Use
`Styxvalue.text(s)` to wrap a Limbo string. `Textual` preserves embedded zero
bytes. `Message.Shared` carries a live cell lease. A typed receive attaches it
before returning. `Message.Intrefdata` carries the original legacy descriptor.
If its issuer is mounted, pass the bytes and the issuer's mount to
[Styxintref.accept](../module/styxintref.m).

Use `sharevalue` to publish a cell and `sendvalue(ref Message.Shared(cell))`
to forward it. `cell.getvalue()` and `cell.setvalue(value)` operate on the
current value regardless of its type. Close an owned lease with `cell.close()`.
Keep operations on one port or cell fid serialized. A rendezvous is blocking.
The old byte-oriented `send`, `recv`, `get`, and `set` remain available.

[Styxmodref](STYX-MODULE-REFS.md) captures and relays exact Int32 exports
of the attached FlashVM on the same named channel.

## Graph contents

`Value.Graph` carries a checked FVMG envelope. Forward it unchanged to
retain the original FlashVM graph metadata. `inspect` exposes portable
own-data records in FVMG versions 8 and 9. It supports native tables,
standard Objects and sparse Arrays with integer logical lengths. Nodes
are addressed by `Value.Link(index)`. Multiple edges may use one index,
and a node can link to itself. Numeric slots and string fields are
distinct on a native table. `inspect` returns nil for unsupported graph
profiles.

`build` writes reachable data as FVMG v8. Supply unique field names and sparse keys. Arrays retain their length, holes and indexed values.
Rebuilt records carry their own data. Prototypes, user methods, watchers,
custom accessors, attributes and native resources are outside this data
view. The standard Array length accessor is included for rebuilt Arrays.

## Executable scenario

The [Limbo test application](../../lib/styxrpc/tests/flashvm/typed_pair.b)
and [AS movie](../../lib/styxrpc/tests/flashvm/typed_pair.as) use the regular
paired executables. To compile and run them, use the
[paired test command](STYX-CHANNEL.md#acceptance) with `--typed`.
The expected marker is `PASS native typed RPC paired VMs`. Run with
`--tcp` to use the loopback TCP path and with `--jit` to enable JIT.

## Reference

- Transport and leases — [sendvalue](styx-named-api/sendvalue.md), [recvvalue](styx-named-api/recvvalue.md), [sharevalue](styx-named-api/sharevalue.md), [getvalue](styx-named-api/getvalue.md), [setvalue](styx-named-api/setvalue.md).
- Codec — [init](styx-named-api/value_init.md), [text](styx-named-api/text.md), [encode](styx-named-api/encode.md), [decode](styx-named-api/decode.md), [inspect](styx-named-api/inspect.md), [build](styx-named-api/build.md).
