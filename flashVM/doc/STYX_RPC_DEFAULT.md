# Default RPC session in paired FlashVM

[Русский](STYX_RPC_DEFAULT.ru.md) · [Callback files](STYX_RPC_NEWNODE.md) · [Limbo client](../../disVM/runtime/STYX-CHANNEL.md)

The paired player gives the movie its already attached duplex session as
`rpc`. The same session carries filesystem access and node publication.
Use [`rpc.ready()`](STYX_RPC_CHAN_READY.md), [`rpc.recv()`](STYX_RPC_CHAN_RECV.md)
and [`rpc.send(value)`](STYX_RPC_CHAN_SEND.md) directly for messages.

The player publishes the hidden `/.rpc` root on this session for the Limbo
peer. One open fid owns that queue until clunk. Player-side `rpc.send`
returns true once queued or false while the port is closed or its output
queue is full. The movie owner operates the local mailbox.

The first SWF arrives at `.upload` over this session. The movie can use
[`rpc.status()`](STYX_RPC_STATUS.md) and the [file methods](STYX_RPC_OS.md).
The `os` global refers to the same object as `rpc`.
