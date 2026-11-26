# ch.recv

[Русский](STYX_RPC_CHAN_RECV.ru.md) · [Open](STYX_RPC_OPEN_EXCHANGE.md) · [Send](STYX_RPC_CHAN_SEND.md) · [Implementation](../src/styxrpc/session_channel.c) · [Hosted exchange](STYX_RPC_DEFAULT.md)

`ch.recv()` returns the next complete value in stream order and accepts
no arguments. It uses one bounded transport poll when the local FIFO
is empty. `undefined` means no complete message is ready, including
when a record spans several polls. A transmitted `null` returns `null`.

For an Int32 reference, assigning the result to an AS variable creates a
live alias. Ordinary reads and writes use the issuing cell. Access and
lifetime follow the original [Int32 reference lease](../../lib/styxrpc/reference/int_variable_refs.md).
The alias holds its own fid, so closing the exchange does not release
an alias that the script still retains.

Call on the movie owner. Although the method polls for messages, its
Styx I/O can wait up to the session request deadline. A transport error
retires the channel. If a complete record cannot be decoded or its cell
cannot be opened, that record fails and is consumed. Other buffered
records remain available.
