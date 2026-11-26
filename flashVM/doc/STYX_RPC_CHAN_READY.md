# ch.ready

[Русский](STYX_RPC_CHAN_READY.ru.md) · [Default RPC session](STYX_RPC_DEFAULT.md) · [Receive](STYX_RPC_CHAN_RECV.md) · [Implementation](../src/styxrpc/session_channel.c)

`ch.ready()` takes no arguments and reports whether a complete record
is already buffered locally. It returns true for any ready payload,
including Boolean false and null. It never polls the connection or
consumes the record. With a client handle, call recv to drive the wire
when its queue is empty.

Call on the movie owner. An invalid argument or closed wrapper raises
an argument error. The borrowed native queue remains owned by the port
or the paired player.
