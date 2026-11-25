# rpc.listen

[Русский](STYX_RPC_LISTEN.ru.md) · [Peer-side newchan](STYX_RPC_NEWCHAN.md) · [Implementation](../src/styxrpc/session_channel.c)

`rpc.listen("/.rpc")` returns a handle to the application exchange published by
the paired FlashVM player. Supply the exact name `.rpc`, with or without
the leading slash. The exchange is registered when the player attaches the
Styx connection. It stays outside ordinary directory reads and file opens.
The peer opens the port through libstyxrpc, for example from
[DisVM using Styxchannel](../../disVM/runtime/STYX-CHANNEL.md).

The handle is available before the peer opens its fid. It borrows the
player's native mailbox and shares it with other handles returned in the
same session. One live peer fid is admitted at a time. The peer can reopen
after clunk. Closing the AS handle releases that wrapper. The peer owns
the lifetime of its fid and closing it discards its pending messages.

On the movie owner, call [ready](STYX_RPC_CHAN_READY.md) to check for a
complete record, then [recv](STYX_RPC_CHAN_RECV.md) to consume it. Hosted
send queues one encoded snapshot and returns true when queued. False
means the peer is absent or the output queue is full. Keep the original
message to retry a false result. The peer reads queued bytes independently.
Hosted send and receive stay local to the mailbox and perform no wire I/O.
Supported values are numbers, Booleans, counted strings and null. The
host-side publication of a live FlashVM variable lease remains future work.

The [paired AS test program](../../lib/styxrpc/tests/flashvm/mixed_pair.as)
uses a frame handler and retains each received value until its response is
queued. Run both application sources using the
[paired acceptance instructions](../../disVM/runtime/STYX-CHANNEL.md#acceptance).
