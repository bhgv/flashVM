# Shared scalar owner service

[Русский](SCALAR-OWNER-SERVICE.ru.md) · [Source](../limbo/scalarowner.b) · [Run two DisVMs](../examples/remote-scalar/STYX.md)

The hosted native modules build includes `scalarowner.dis`, a single
service for one published `fn(int): int` export. It runs in the owner's
VFS. It uses [Vfsnative.scalarload](VFS-SCALAR-LOAD.md) to bind the
implementation, then calls the linked slot through the normal Dis
instruction. Request and reply transport run in the shared C builtin.

The trusted launcher supplies six or seven tab-separated UTF-8 fields via
`Launchcontext.arguments()`. The complete string is limited to 4096 bytes.
Fields appear in this order.

| Field | Meaning |
| --- | --- |
| Implementation | Path of the original Dis library in the owner's namespace |
| Export name | Name of the function in the compiled export table |
| Request | Read path of the admitted request inbox |
| Reply | Write path of the reverse Styx file |
| Request tag | Decimal nonzero 32-bit route tag |
| Reply tag | Decimal nonzero 32-bit return tag |
| Call count | Optional decimal integer from 0 to 65536, default 1. Zero waits for session end |

The service loads the implementation once and handles the configured number of
requests. Each request receives one value or error record.
An exception from the implementation maps to owner error code 1.
Paths and tags are assigned when the host grants the route. The service
does not dial, mount a peer or select an implementation from wire data.

With a zero count, the owner waits for calls until a close record or EOF.
A positive count preserves the bounded mode.
