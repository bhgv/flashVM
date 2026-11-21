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

## Several exports of one implementation

Start the same service with tab-separated fields in this order:

```text
set  implementation  export  request  reply  [export request reply ...]
```

Supply one to eight export triples. Each request and reply path points to the
corresponding local mailbox from [the mounted server](../native/terminal/SCALAR-FUNCTION-MOUNTED.md).
The service uses request tag 72 and reply tag 73 for all triples. It links all
names to one instance before reading requests. Module globals are shared across
calls to any of its exports. The service checks routes in configuration order,
waits briefly when all live routes are idle, and exits after every request
mailbox reaches EOF. The single-export startup format above remains available.

The two-DisVM example serves `compute` and `adjust` from the same implementation.
Alternating the functions, including a call that raises, verifies that both
observe the same counter. See [the runnable mounted example](../native/terminal/SCALAR-FUNCTION-MOUNTED.md).

## One referenced ADT export

For a mounted export with the [scalar-field ref profile](../native/terminal/SCALAR-FUNCTION.md#copy-back-a-referenced-adt),
the publisher sets `dis_ref` on its `StyxMountedScalar` entry. Start the shared
service with five tab-separated fields.

```text
ref  implementation  export  request  reply
```

The service binds the named export once and waits for requests until EOF.
For each request it allocates an owner-local ADT from the input snapshot
and invokes the linked export. On success the service sends the return
tuple and the ADT's final scalar fields. An exception sends an error record
and the caller keeps its original field values. The startup format uses
the unscoped request. See the [runnable example](../examples/remote-scalar/STYX.md#copy-back-from-an-original-dis-export).
