# rpc.recv

[Русский](STYX_RPC_CHAN_RECV.ru.md) · [Send](STYX_RPC_CHAN_SEND.md) · [Implementation](../src/styxrpc/session_channel.c) · [Default session](STYX_RPC_DEFAULT.md)

`rpc.recv()` takes no arguments and returns the next buffered AS Number,
Boolean, counted string or `null` in arrival order. An empty queue returns
`undefined`. The movie owner reads its local mailbox and does no wire I/O.
An unsupported or invalid record is consumed and raises an argument error.
