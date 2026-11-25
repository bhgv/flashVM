# ch.close

[Русский](STYX_RPC_CHAN_CLOSE.ru.md) · [Open](STYX_RPC_NEWCHAN.md) · [Implementation](../src/styxrpc/session_channel.c)

`ch.close()` releases the channel's exchange fid, drops unread records
and returns `undefined`. It takes no arguments. Repeated calls succeed.
Subsequent send and receive calls report an argument error.

Call on the movie owner. Closing a live port may wait for the Styx
clunk deadline. Garbage collection of an abandoned channel defers the
clunk to a later session operation. A received live alias owns its own
fid and follows its original lease.
