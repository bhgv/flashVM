# Named channel recv()

[Русский](STYX_RPC_NAMED_RECV.ru.md) · [vfs.chan](STYX_RPC_NAMED_CHANNEL.md) · [Implementation](../src/styxrpc/session_named_channel.cpp)

Wait for the oldest pending writer on this named channel.

## Syntax

```actionscript
port.recv()
```

## Availability

Available in the StyxRPC-enabled player and paired detached tasks when
the session provides named channels. `port` is the object returned by
[`vfs.chan(path)`](STYX_RPC_NAMED_CHANNEL.md). The method takes no arguments.

## Return value and errors

A regular message returns a decoded value. A live cell descriptor
issued by this session attaches a new lease and returns a transparent
reference. Reading or assigning through the reference accesses the cell.
Its lease can remain live after the channel participant closes.

A closed channel, malformed frame, failed attachment or broken connection
raises an exception. A movie-owner call can wait between frames. In an
[asinc task](ASINC_TASK.md), [`cancel()`](ASINC_CANCEL.md) can also stop
attachment to the incoming cell. A successful rendezvous may already
have consumed the descriptor before cancellation.

A remote module Int32 reference opens the original export on its first read
or assignment. These operations support cancellation in an `asinc` task.
References to local exports use the movie context.
[Runnable FlashVM and DisVM example](../../lib/styxrpc/examples/disvm/INT32_REFERENCES.md).

## Example

The sender publishes a numeric cell with
[`send(ref counter)`](STYX_RPC_NAMED_SEND.md). The receiver reads and
updates that numeric value. Run the sender on another execution thread
or in the peer VM.

```actionscript
var port = vfs.chan("/jobs");
var counter = port.recv();
var previous = counter + 0; // Read the shared value here.
counter = previous + 1;   // Assign through the received reference.
var released = port.delete(); // Release this channel participant.
```

Both leases see the increment after assignment is acknowledged. Writers
that need an atomic read/modify/write sequence must coordinate it at
the application level.

[Shared cell contract](../../lib/styxrpc/reference/named_cell.md) ·
[Complete C and Limbo example](../../lib/styxrpc/examples/shared-cell/README.md)
