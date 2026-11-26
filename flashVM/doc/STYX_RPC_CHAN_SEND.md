# rpc.send

[Русский](STYX_RPC_CHAN_SEND.ru.md) · [Receive](STYX_RPC_CHAN_RECV.md) · [Implementation](../src/styxrpc/session_channel.c) · [Default session](STYX_RPC_DEFAULT.md)

`rpc.send(value)` queues one AS Number, Boolean, counted string of up to
16,384 bytes, or `null` on the player's default exchange. Use it on the
movie owner thread. It returns `true` after the SDK copies the payload,
or `false` when the peer's message port is closed or the queue is full.
It does no synchronous wire I/O. An unsupported payload or argument
count raises an argument error.
