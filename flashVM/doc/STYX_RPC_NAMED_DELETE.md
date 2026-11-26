# Named channel delete()

[Русский](STYX_RPC_NAMED_DELETE.ru.md) · [vfs.chan](STYX_RPC_NAMED_CHANNEL.md) · [Code](../src/styxrpc/session_named_channel.cpp)

`port.delete()` releases the object's participation and returns
`undefined`. It accepts no arguments and can be called again safely. The
channel remains available until its last participant retires. Each later
`vfs.chan(path)` creates a fresh channel if the path has become free.
