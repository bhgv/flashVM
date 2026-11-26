# Named channels between two FlashVMs

[Русский](STYX_RPC_PEER_CHANNELS.ru.md) · [Paired player](STYX_RPC_PLAYER.md#connect-two-flashvms) · [ActionScript API](STYX_RPC_NAMED_CHANNEL.md)

Two current FlashVM peers enable named rendezvous on the attached connection
when they exchange the named-service capability in Styx attach. The role
`forward` hosts one libstyxrpc registry for both peers. The role `reverse`
mounts its service on an independent logical stream of the same carrier.

Both `vfs.chan(path)` and `os.chan(path)` use that registry. An absolute
path starts at its root and a relative path uses the caller's current
`vfs.pwd()` before joining. The channel namespace follows the forward
player's live callback tree. The root and existing traversable callback
directories admit free channel names. Ordinary files, directories,
`mounted` and reserved service paths refuse enrollment. A channel
reservation blocks callback publication at the same path and is hidden
from ordinary directory reads. If a parent is replaced, existing participants
can finish through their fids. New joins succeed after the old name retires.

The matching `send` and `recv` calls block. Native dispatchers advance the
rendezvous and cell operations while a movie is waiting. Its frames and
ordinary AS callbacks run on the movie owner once it is available. Closing
the last channel participant releases its name. A live shared cell keeps
its own lease. Connection loss cancels pending exchanges.

Numbers, strings, booleans, null, undefined and supported graphs pass
by value. Capture a writable AS slot with `ref` to pass a live cell.
Ordinary reads and assignments at either end use that cell. A second
`send(ref alias)` forwards its identity. Int32 descriptors of module
variables belong to their original VFS attachment. The common named
service handles AS slots and its own shared cells.

## Run acceptance

Use the [paired player check](STYX_RPC_PLAYER.md#run-acceptance). It builds
the [caller](../tests/styxrpc/peer-named-caller.as) and
[provider](../tests/styxrpc/peer-named-provider.as) with AS2, FIR and SWL.
The movies exchange a graph with repeated references and a cycle, update
shared slots from each side, then read them after closing the channel.
The provider waits for another value when the test disconnects the caller.

The stock DisVM pair uses its named service on the existing mount.
Applications embedding the C SDK can use the
[hosted named peer](../../lib/styxrpc/reference/named_peer_host.md).
