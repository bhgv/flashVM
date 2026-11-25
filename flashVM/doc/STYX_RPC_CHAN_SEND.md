# ch.send

[Русский](STYX_RPC_CHAN_SEND.ru.md) · [Open](STYX_RPC_NEWCHAN.md) · [Receive](STYX_RPC_CHAN_RECV.md) · [Implementation](../src/styxrpc/session_channel.c)

`ch.send(value)` sends a snapshot of an AS Number, Boolean, counted string
(up to 16,384 bytes), or `null`. `ch.send(ref counter)` forwards a live Int32
reference previously received from the same Styx session. The original
identity and permissions are preserved. Local AS variables need an
exported cell before they can be shared. This API currently issues no
descriptors for local AS variables.

The selected [wire schema](../../lib/styxrpc/reference/channel_messages.md)
follows the value. Passing an alias as `ch.send(counter)` performs the
ordinary AS read and sends its current value. Type negotiation and
automatic conversion based on the receiver's preferred type are pending.

Call from the movie owner. The method writes through Styx using the
session deadline and returns `true` after acknowledgement. It performs
no receive dispatch. Unsupported values and closed handles produce an
argument error. Transport failure retires the handle and reports an
error. The peer may already have seen a failed write, so repeating it
can duplicate an effect.
