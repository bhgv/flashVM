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

## One argument-list export

Publish a `StyxMountedScalar` entry with `dis_call` for the
[argument-list profile](../native/terminal/SCALAR-FUNCTION.md#argument-lists-and-nested-returns).
Pass five tab-separated fields to the same shared service.

```text
list  implementation  export  request  reply
```

The service verifies the compiled signature once when it links the
original Dis instance. For each request it invokes the export with three
ordinary Limbo arguments and sends the nested return or an error. It runs
until the request mailbox reaches EOF.

## One export with two references

Publish a `StyxMountedScalar` entry with `dis_ref_pair` for the
[two-ref profile](../native/terminal/SCALAR-FUNCTION.md#two-references-in-one-call).
Pass five tab-separated fields to the shared service.

```text
refpair  implementation  export  request  reply
```

The service links the original export once. Each request creates one
owner-local ADT per object ID. When the two IDs match, the ordinary Dis
call receives the same reference twice. On success it returns the result
and each object's final state once. An exception sends an error through
the reply mailbox. The request mailbox reaching EOF ends the service.

## Profile from the original load

Use [styxunibusserverstartloaded](../native/terminal/UNIBUS-SERVER-LOADED.md)
to reserve one export before the owner's VM starts. Supply the following
six tab-separated fields to the shared service.

```text
loaded  implementation  export  request  reply  offer
```

`implementation` and `export` select an original Dis image and its
named function. `request` grants read access to the local request inbox.
`reply` grants write access to the local response inbox. `offer` grants
write access to the [private profile mailbox](../native/terminal/UNIBUS-SERVER-LOAD-PROFILE.md).

An owner that starts from an ordinary image can
[discover the callable names on its loaded instance](VFS-SCALAR-LOADED-EXPORTS.md).
The catalogue gives it the exact candidates for constructing a publication
without a caller-supplied name list. The existing library server still accepts
its configured list of exports.

The normal Dis loader reads the local image and the linker creates a
module instance for the selected export. The shared service reuses that
instance and sends the compiled export name and signature to the C
dispatcher through `offer`. The dispatcher selects the matching gateway
and makes the same S-record available as `.interface`. Ordinary client
`load` compares it with its compiled import without reading a source
`.m` over the network. This mode covers one export. It accepts the six
fixed profiles and [additional scalar shapes](../native/terminal/SCALAR-FUNCTION.md#calls-with-compiled-scalar-types).
The loaded directory can mix both kinds of export.

## One load for several exports

Use [styxunibusserverstartloadedset](../native/terminal/UNIBUS-SERVER-LOADED-SET.md)
to reserve a directory of selected exports. Supply these tab-separated
fields in the names array's original order.

```text
loaded-set  implementation  export  request  reply  offer  [export request reply offer ...]
```

Supply one to eight export groups. Each group has the same permissions
as the single-export `loaded` mode. The service loads the original
image once and creates one local module data instance. Each one-slot
typed view holds a reference to the same module data. All views are linked
before starting their request loops. Missing or unsupported exports refuse
the directory before the client can call any function. Native Styx
dispatch on each server remains sequential. The service waits for its
request loops to reach EOF before exiting.

The [two-VM example](../examples/remote-scalar/STYX.md#load-six-exports-that-share-state)
checks all six fixed profiles in one directory.
