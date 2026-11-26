# vfs.chan(path)

[Русский](STYX_RPC_NAMED_CHANNEL.ru.md) · [Default session](STYX_RPC_DEFAULT.md) · [FlashVM implementation](../src/styxrpc/session_named_channel.cpp) · [Wire protocol and Limbo](../../lib/styxrpc/reference/named_channel.md)

`vfs.chan(path)` opens a named channel on the paired session. `os.chan(path)`
uses the same session. An absolute path starts at the session root. A relative path starts at
[`pwd()`](STYX_VFS_PWD.md). Each call owns one participant. If the name is free, the
current namespace receives a channel binding. In DisVM its parent must permit writing
and traversal. An existing ordinary file, directory, or reserved name causes
an error. An existing channel admits another participant.

[Two FlashVMs](STYX_RPC_PEER_CHANNELS.md) share the named namespace of the
forward player, including its name reservations and cell leases.

| Method | Effect |
| --- | --- |
| [`recv()`](STYX_RPC_NAMED_RECV.md) | Waits for a writer and returns its value. |
| [`send(value)`](STYX_RPC_NAMED_SEND.md) | Waits for a reader to accept the value. |
| [`delete()`](STYX_RPC_NAMED_DELETE.md) | Releases this participant. |

The last release removes the exact VFS binding. GC of an unreachable object
queues the same release, and ending the attachment retires its fids.
An extracted method retains its channel object. The ordinary FlashVM file
API hides the service and refuses access to a channel name.

```actionscript
var work = vfs.chan("/jobs");
var job = work.recv();
var sent = work.send(job);
var released = work.delete();
```

Run the corresponding producer or consumer on another execution thread or in
the peer VM. In the stock paired player, a call on the movie owner suspends
frame advancement until it returns. DisVM can continue while the movie waits.
The player acknowledges `.state start` before executing the first frame.
