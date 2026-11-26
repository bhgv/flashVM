# vfs.publishchan

[Русский](STYX_RPC_PUBLISHCHAN.ru.md) · [Default session](STYX_RPC_DEFAULT.md) · [Implementation](../src/styxrpc/session_channel.c)

`vfs.publishchan(address, callback)` reserves the syntax for accepting a
new connection. Supply a nonempty address string without
NUL, for example `tcp!port`. The callback will prepare the accepted
connection's VFS.

The current method checks the address string and callback object, then
returns `undefined`. It creates no socket, connection or VFS root and
neither invokes nor retains the callback.
