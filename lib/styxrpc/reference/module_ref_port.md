# Positioned module reference port

[Русский](module_ref_port.ru.md) · [Paired module references](module_refs.md) · [Limbo example](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/STYX-MODULE-REFS.md)

Use a reference port when an embedding needs to expose an exported
Int32 through a byte-oriented handle. The port retains the attached
issuer and applies the shared module-reference codec, permissions and
Qid check. Each port binds one export. The expected owner role and
connection scope come from the pairing, and an application can close
its channel once it holds the port.

| Operation | Position | Bytes |
| --- | --- | --- |
| Capture | Write at 0 on an unbound port | Opcode `1`, mode `0` or `1`, then the canonical relative path without NUL |
| Adopt | Write at 0 on an unbound port | Opcode `2`, followed by the complete named-channel frame including tag 9 |
| Get | Read at 0 on a bound port | Exactly 4 bytes, signed Int32 in big-endian order |
| Set | Write at 0 on a writable bound port | Exactly 4 replacement bytes in big-endian order |
| Forward | Read at 4 or greater on a bound port | Counted by read extent. The full descriptor starts at 4 and ends at EOF. |

Capture opens the original remote fid and constructs a descriptor from
its Qid. Adopt copies and verifies a descriptor without calling the
issuer. Use [paired open](module_ref_port_open_paired.md) to also admit
descriptors for an export owned by the local endpoint.
The first Get or Set on an adopted port opens the exact export and
checks its Qid. If that export has been withdrawn or replaced, the
request fails. Forward reads only the cached frame.

The ordinary reverse DisVM mount publishes the scope at `/.varref`
as a read-only service. The DisVM guest adapter uses the port commands
on its independent read/write service fd. An external C application
can use the same port interface directly over an authorized mount.

[Open](module_ref_port_open.md) · [Paired open](module_ref_port_open_paired.md) · [Read](module_ref_port_read.md) · [Write](module_ref_port_write.md) · [Close](module_ref_port_close.md)
