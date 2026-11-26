# rpc.chan(path)

[Русский](STYX_RPC_NAMED_CHANNEL.ru.md) · [Default session](STYX_RPC_DEFAULT.md) · [FlashVM implementation](../src/styxrpc/session_named_channel.cpp) · [Wire protocol and Limbo](../../lib/styxrpc/reference/named_channel.md)

`rpc.chan(path)` opens a named channel in the paired DisVM namespace. `os.chan(path)`
uses the same session. The path starts at the session root with or without a
leading slash. Each call owns one participant. If the name is free, the
current namespace receives a channel binding. Its parent must permit writing
and traversal. An existing ordinary file, directory, or reserved name causes
an error. An existing channel admits another participant.

| Method | Effect |
| --- | --- |
| [`read()`](STYX_RPC_NAMED_READ.md) | Waits for a writer and returns its value. |
| [`write(value)`](STYX_RPC_NAMED_WRITE.md) | Waits for a reader to accept the value. |
| [`delete()`](STYX_RPC_NAMED_DELETE.md) | Releases this participant. |

The last release removes the exact VFS binding. GC of an unreachable object
queues the same release, and ending the attachment retires its fids.
An extracted method retains its channel object. The ordinary FlashVM file
API hides the service and refuses access to a channel name.

```actionscript
var work = rpc.chan("/jobs");
var job = work.read();
var sent = work.write(job);
var released = work.delete();
```

Run the corresponding producer or consumer on another execution thread or in
the peer VM. In the stock paired player, a call on the movie owner suspends
frame advancement until it returns. DisVM can continue while the movie waits.
The player acknowledges `.state start` before executing the first frame.
