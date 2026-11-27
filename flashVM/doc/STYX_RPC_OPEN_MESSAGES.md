# styxrpc.openMessages

[Русский](STYX_RPC_OPEN_MESSAGES.ru.md) · [Session](STYX_RPC_SESSION.md) · [Implementation](../src/styxrpc/session_messages.c)

To publish a root on the paired FlashVM side, use [publishMessages](STYX_RPC_PUBLISH_MESSAGES.md).

```actionscript
var port = styxrpc.openMessages("events", ["app.sample", 1, 47, "onSample"]);
```

Pass one published root name from the session's attach root or `null` for
`.unibus`. Add one to 32 dense descriptors, each containing a printable ASCII
name of 1–63 bytes, a nonzero UInt32 version, a UInt32 send tag, and optionally
the receive method name. Omit the method for send-only registrations. Use
tag zero for receive-only registrations. Each sending tag is unique within
this direction and port. The opposite side selects its own tags.

The call can wait for Styx I/O. It returns an object that owns the port.
Malformed descriptors and unavailable roots raise an error. The script can
open roots published by the peer through
[libstyxrpc](../../lib/styxrpc/reference/messages_publish.md).

| Port method | Contract |
| --- | --- |
| `send(name, version, bytes)` | Copy a dense array of exact integers 0–255, up to 16384 bytes. Return 0 when queued, 1 when the queue is full and −1 when refused. Transmission begins at the next pump. |
| `pump(receiver)` | Flush queued output and read one bounded batch. After transport I/O, invoke each registered method as `receiver.method(bytes)` on the movie owner. Return the number delivered. A wire failure or propagated callback error retires the port and discards the remainder. Callbacks may queue sends. Pump and close require an idle port. |
| `close()` | Clunk the idle port, discarding queued output. Repeated close succeeds. Collection defers the clunk and frees native state. |

The [UniBus contract](../../lib/styxrpc/reference/custom_messages.md) describes
version and tag negotiation. Both default and custom exchange roots are
hidden from [ordinary file operations](STYX_RPC_SESSION.md).
