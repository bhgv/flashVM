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
`send(ref alias)` forwards its identity.

## Pass an exported variable

On a pair of FlashVMs that support module references, open a published Int32
such as `/mounted/ref-peer.dis/.var/counter` with `vfs.intVariable(path, true)`.
The second argument requests write access. Omit it for a read-only view.
Once the module is published and its owner is serving VFS, the importing
movie can send the property reference and receive it back.

```actionscript
var proxy = vfs.intVariable("/mounted/ref-peer.dis/.var/counter", true);
var pipe = vfs.chan("/module-ref-work");
var sent = pipe.send(ref proxy.value);
var returned = pipe.recv();
var closed = pipe.delete();
```

The movie that published the counter joins the same named channel,
receives the reference, updates its own export through it and sends it back.

```actionscript
var pipe = vfs.chan("/module-ref-work");
var alias = pipe.recv();
alias = -73;
var sent = pipe.send(ref alias);
var closed = pipe.delete();
```

After this exchange, `returned` and `proxy.value` both read `-73`.
Assigning an exact signed Int32 to `returned` changes the original export.
The alias can outlive the channel endpoint. Its rights follow the opening
that produced it. The owner opens a local alias against its current slot
on receipt. A remote alias opens and checks its original fid on first access.
The owner must be available to serve that access. Withdrawing or replacing
the module invalidates the old references, even if the same name is published
again. The pair negotiates this feature on its named service stream.
[C SDK and wire format](../../lib/styxrpc/reference/module_refs.md).

## Run acceptance

Use the [paired player check](STYX_RPC_PLAYER.md#run-acceptance). It builds
the [caller](../tests/styxrpc/peer-named-caller.as) and
[provider](../tests/styxrpc/peer-named-provider.as) with AS2, FIR and SWL.
The movies exchange a graph with repeated references and a cycle, update
shared slots from each side, then read them after closing the channel.
The provider waits for another value when the test disconnects the caller.

The same check builds the [module caller](../tests/styxrpc/peer-module-refs-caller.as),
[provider](../tests/styxrpc/peer-module-refs-provider.as) and
[readiness callback](../tests/styxrpc/peer-module-refs-start.as). They relay
a writable and a read-only reference and verify that both reflect updates
to the original export after the channel closes.

The stock DisVM pair uses its named service on the existing mount. Limbo
can [relay exported Int32 variables of either VM](../../disVM/runtime/STYX-MODULE-REFS.md).
Applications embedding the C SDK can use the
[hosted named peer](../../lib/styxrpc/reference/named_peer_host.md).
