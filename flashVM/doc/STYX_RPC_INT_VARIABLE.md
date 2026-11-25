# styxrpc.intVariable

[Русский](STYX_RPC_INT_VARIABLE.ru.md) · [Session](STYX_RPC_SESSION.md) · [Implementation](../src/styxrpc/session_modules.c)

```actionscript
var counter = styxrpc.intVariable("/mounted/peer.dis/.var/count", true);
counter.value = 77;
var confirmed = counter.value;
```

Pass the exact path of an Int32 cell under `.var`, optionally with a leading slash.
Omit the Boolean or pass `false` for read-only access. The returned object owns
the opened fid. Reading `value` fetches the current signed Int32 from the
owner. A writable proxy accepts exact integer numbers from −2³¹ through
2³¹−1. Failed access raises an exception. A failed write can report an uncertain
outcome if the acknowledgement was lost. Reads and writes run on the movie owner
and can wait for I/O. Garbage collection defers the fid's clunk. Ordinary
file operations continue to hide `.var`. See [publishObject](STYX_RPC_PUBLISH_OBJECT.md).
