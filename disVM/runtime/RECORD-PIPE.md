# Vfsexport.recordpipe

[Русский](RECORD-PIPE.ru.md)

A record pipe lets a Limbo process exchange messages with a peer that sees
a granted subtree over Styx. Both files are virtual. File operations stage
whole S-records and read receipts. An application can send the decoded
value into a local Limbo channel before acknowledging receipt.

## Interface

```limbo
include "vfsexport.m";
export := load Vfsexport Vfsexport->PATH;
pipe := export->recordpipe();
```

`recordpipe()` takes no arguments. It returns an owned `ref Vfsexport->Root`
or `nil` on allocation failure. Use `Vfsexport->open` to open an endpoint
from the capability directly. A mounted peer can use `Sys->open`. Both ends
require `Sys->ORDWR`. Use [`Vfsexport->subtree`](VFSEXPORT-SUBTREE.md)
to capture only the end the peer needs. Map that exact file by number in
the outgoing descriptor grant with `Vfsexport->overlay`. The peer opens
`.prog/<pid>/.fd/<number>` over Styx.

| Open file | Write | Read |
| --- | --- | --- |
| `send` | One complete S-record of 1–1024 wire bytes | One receipt byte after the receiver responds |
| `recv` | One receipt byte on the same fid that read the record | The complete waiting S-record |

The sender submits one record and keeps its fid open until it reads the
receipt. Only one transaction can occupy a pipe. A write while occupied
fails. A malformed record fails without occupying the pipe. The receive
buffer must hold the entire wire record. A short buffer fails without
taking any bytes. A read returns zero while its endpoint has nothing
ready. Reads and writes never wait for the other endpoint.

The receiver writes byte `1` to confirm delivery or `0` to reject it.
Closing the receiving fid after taking a record and before acknowledging
it reports byte `2` to the sender. That outcome is uncertain. The pipe
does not deliver the record again. Closing the sending fid removes its
outstanding transaction. If the receiver already took the message, it
can no longer acknowledge that transaction. A late receipt fails and
has no effect on the next message. The pipe becomes available
after the sender takes a receipt or closes its fid.

The pipe validates S-record framing and checksum. The endpoint that
interprets the value validates its type and payload. This interface
provides explicit file attachment to a Limbo handler. It can be used on a
native-only build or through a scoped reverse Styx connection in an
SDK-enabled build.

## Example

The three-VM fixture publishes one endpoint in each direction as
`.fd/30` and `.fd/31` beneath the caller's verified PID. The other ends
are held privately by the relay. Its relay accepts a message in a local `chan of int`
via `alt`, acknowledges delivery and sends the typed reply back through
the second pipe. The adverse cases cover a damaged record, the wrong
open mode, a hidden endpoint and confirmation on the wrong fid.
[Run the three-VM example](../examples/remote-scalar/STYX.md#run-on-separate-hosts).
[Limbo interface](../module/vfsexport.m).
[Native implementation](vfs/record_pipe.c).
