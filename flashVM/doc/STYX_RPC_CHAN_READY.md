# rpc.ready

[Русский](STYX_RPC_CHAN_READY.ru.md) · [Default RPC session](STYX_RPC_DEFAULT.md) · [Receive](STYX_RPC_CHAN_RECV.md) · [Implementation](../src/styxrpc/session_channel.c)

`rpc.ready()` takes no arguments and reports whether a complete record is
buffered locally, including Boolean false and null. It neither polls the
connection nor consumes the record. Call on the movie owner thread.
An unexpected argument raises an argument error.
