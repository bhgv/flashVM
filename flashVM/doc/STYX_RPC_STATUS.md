# vfs.status()

[Русский](STYX_RPC_STATUS.ru.md) · [Session](STYX_RPC_DEFAULT.md) · [Implementation](../src/styxrpc/session_launch.c)

Call `vfs.status()` or `os.status()` with no arguments in the paired
player. Both return a new snapshot with a Boolean `connected` and a
`channel` string. The channel is `"open"` while the local Styx client
reports a live attachment and `"closed"` after it reports disconnection.
The call does no I/O. A failure the client has yet to observe can leave
the snapshot reporting an open channel.

```actionscript
var session = vfs.status();
if (session.connected) {
    var input = os.open("/notes.txt");
    input.close();
}
```

Use [`vfs.ready()`](STYX_RPC_CHAN_READY.md) to check for a pending message
and [`vfs.status(token)`](STYX_RPC_SESSION.md) for a named launch receipt.
