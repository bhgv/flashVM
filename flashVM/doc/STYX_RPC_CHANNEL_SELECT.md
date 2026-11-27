# select and trySelect on a live UniBus channel

[Русский](STYX_RPC_CHANNEL_SELECT.ru.md) · [Live channels](STYX_RPC_CHANNELS.md) · [Implementation](../src/styxrpc/channel_select.c)

```actionscript
anchor.select(guards)
anchor.trySelect(guards)
```

Use an open remote channel as `anchor`. Pass a dense Array of 1 through 8 guards. A guard is a dense Array containing `[channel]` for receive or `[channel, value]` for send. The script value must fit the destination channel's compiled element schema. Guard Arrays use own indexed data slots. The bridge reads those slots without invoking script.

All guard channels must be open and on the anchor's exact UniBus. They can share a token of an imported module result, the PID of a scoped incoming call, or one imported PID of a hosted result mailbox. The anchor itself may be absent from the list. Separate calls, separate tokens and separate PIDs require separate selections. Mixed channel types and admissible nested channels use the same wire format as individual operations.

One request goes through libstyxrpc to the bus owner. The owner chooses at most one guard. `select()` waits for a choice within the host's configured operation budget. `trySelect()` requests immediate selection. When no guard is ready, `trySelect()` returns `undefined`. With a confirmed choice, both methods return `[ordinal, value]`. The ordinal is one-based in submission order. A send uses `undefined` in the value slot. A receive may return `null` for a nil channel.

A malformed guard list reports an argument error. A closed channel, a guard on another bus or an incompatible send value is refused before submission. After submitting, a timeout or transport error can race with a committed branch. Both methods run synchronously on the owner context and use one temporary fid. The route retains the lifetime of the original call or result. A lost confirmation can leave the outcome uncertain, so callers should avoid automatic retries.

In a detached task, [`cancel()`](ASINC_CANCEL.md) closes the selection's
operation fid between Styx round trips. A branch already committed by
the bus owner can have an effect even when the task receives no result.

## Receive or send on the same result bus

```actionscript
var chosen = ints.trySelect([[ints], [messages, "ready"]]);
if (chosen != undefined) {
    if (chosen[0] == 1) {
        trace(chosen[1]);
    }
}
```

Here `ints` and `messages` come from one module result. The first guard receives an integer. The second sends a string. A selection of branch 2 confirms that the string was sent. An idle reply means neither branch ran.

The [native fixture](../tests/styxrpc/channel.c) exercises mixed integers and counted strings, same-token nested values, and an idle selection. The same methods work in [incoming callbacks](../tests/styxrpc/channel-incoming.c) and on [PID mailbox channels](../tests/styxrpc/channel-pid-result.c). [C selection decoder](../../lib/styxrpc/reference/channel_result.md)
