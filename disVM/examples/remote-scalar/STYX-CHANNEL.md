# Pass a Limbo channel through Styx

[Русский](STYX-CHANNEL.ru.md) · [Mounted library guide](STYX.md)

An ordinary `load` can link a library export that takes a live `chan of T`.
The caller opens the mounted function as usual. DisVM publishes a virtual
`.prog/<pid>/.unibus` file in the caller's reverse Styx grant, then sends one
capability key for each distinct non-nil channel argument. Repeating an
argument preserves channel identity in the provider. The provider can execute
the original Dis function with its normal channel instructions. A thread
spawned by the provider can use those same proxies while the call is live.
If the provider returns a channel granted by the caller, the caller receives
its original channel object, with the same queue and buffering. The caller can
grant a channel in the initial arguments or inside a later channel message.
A function can also return a new provider-owned channel. The caller uses the
returned handle through ordinary Limbo channel instructions.

## Run the example

Build the SDK and SDK-enabled runtime as described in the
[mounted library guide](STYX.md#prepare-and-exercise-the-publication).
Run from the repository root with the resulting DisVM and Limbo paths.

```sh
python3 disVM/examples/remote-scalar/run_channel.py \
    --runtime build/scalar-runtime/disvm \
    --limbo build/scalar-runtime/bootstrap/limbo-bootstrap
```

Add `--jit` to run both guests in JIT mode. The runner compiles the client and
provider with `-D RG`, starts two DisVM processes in separate temporary
directories and connects them over TCP loopback. It checks for
`PASS automatic Limbo channels over UniBus` and removes the images on exit.

| Source | What it demonstrates |
| --- | --- |
| [Interface](chain-channel.m) | Regular typed parameters and a channel result. |
| [Client](styx-channel-client.b) | Normal `load`, a large tuple, channels inside messages, cancellation and reuse, and returned channels with their original identities. |
| [Provider](styx-channel-owner.b) | Direct channel instructions in the original process and in `spawn`, selection across sparse arrays of proxy channels, and local and remote guards with a fallback. |

Run [run_channel_pair.py](run_channel_pair.py) with the same options to
exercise two provider threads rendezvousing on one unbuffered proxy while
the caller waits for the function reply.

Run [run_channel_lifetime.py](run_channel_lifetime.py) with a case name and
the same options.

```sh
python3 disVM/examples/remote-scalar/run_channel_lifetime.py mixed \
    --runtime build/scalar-runtime/disvm \
    --limbo build/scalar-runtime/bootstrap/limbo-bootstrap
```

| Case | What it checks |
| --- | --- |
| `mixed` | Blocking local and remote branches, two mixed selections rendezvousing on one UniBus, and cancellation. |
| `result` | Returning a new provider channel through `load`, using it after releasing the module, selecting across independent leases, forwarding message channels in both directions, and rendezvous through a regranted channel. |
| `recursive` | New grants through a channel that was itself granted in a message. It alternates directions, checks aliases and recycles the routes. |

## Channel operations

The VM waiting for the function reply services UniBus requests using the
original channel queues. A send completes when a receiver takes the value or
when the original buffer has room. A receive uses the original buffer or a
waiting sender. When two operations on the same UniBus are waiting on
opposite ends of one unbuffered channel, the bus pairs their fids after checking
the original channel for local waiters. It publishes both replies under the
same lock and then returns the fallback for any unmatched immediate selection.

A provider `alt` with remote guards submits them together and commits one
guard. A receive array contributes one guard for each non-nil channel.
The result preserves the original branch and array index. An `alt` with a
fallback can also mix local and remote guards. It tests each eligible option at
most once in randomized order and stops at the first match. A blocking mixed
`alt` registers local guards in the normal Dis queues and periodically withdraws
them under the same arbitration lock used by their peers. It can then submit
remote guards on one UniBus route for a bounded window of up to 40 ms at the
owner. Two such windows can rendezvous on that bus. The VM resumes local
selection only after an authoritative remote reply. Once a local or remote
guard commits, that branch supplies the result of the selection.

An `alt` with proxy guards on independent UniBus routes groups them by route.
It advances to the next route only after the current owner confirms that
no guard matched. With a fallback it probes each route once. In blocking
mode it repeatedly gives each route a bounded offer window. This lets the
selection wait across multiple routes while keeping one offer live at a time.

Each operation opens a fresh read/write fid on the existing reverse Styx
session. The operation and its reply share that fid. A complete record can span
several Styx messages. The operation becomes visible only after the full record
passes framing and type validation. On cancellation or return the caller
seals its UniBus endpoint and drops the grant. The provider revokes the route
in its spawned threads at the same boundary. If transport fails after a match
commits, the provider receives an error and the VM does not repeat the operation.

Channel elements use the managed value codec for numbers, strings, arrays,
tuples, supported `ref` values and channel handles. Ordinary `ref` aliases
are local to one message. Channel identity is preserved across messages and
provider threads in the same call. The provider can send back handles it
received on this route. The bus checks every such key against the caller's
published types before admitting a send.

For new channels returned by the provider, the caller uses the module's
`.unibus` under `mounted/<module>` on the existing Styx connection. The
return record includes an opaque token for the group and one key per
channel. Each operation opens its own fid and sends the token before its
S-record. The original function fid stays open while any proxy in the group
is live, including after releasing the loaded module. Disposing of the last
proxy clunks that fid and releases the provider's channel service.

A returned proxy can be passed as an argument to a later remote call. The
caller's reverse UniBus publishes a scoped key for it and forwards operations
to the original owner. Echoing that key in the call result restores the
caller's original proxy. For a known unbuffered origin, the reverse bus
first tries the original route. It can then match two pending fids in the
same call under its bus lock when the owner has no ready partner.

A provider-owned channel can carry channels in a typed message. The owner
adds new keys to the same lease before publishing a receive reply. The
client can send a local channel, a channel from another lease, or a key
already issued on the current lease. For a new reverse grant the client
publishes a virtual `.prog/<id>/.unibus` on the same Styx connection. The
identifier is reserved outside the guest PID range. The provider uses a
proxy to the original queue. Sending that proxy back on the same lease
resolves to the original client object. Repeated keys share identity while
their proxy is live.

An imported channel can itself carry a new grant. In the mounted-module
direction, its owner reserves a child token in the same module's `.unibus`
registry. In the reverse direction, its owner publishes a synthetic
`.prog/<id>/.unibus` on the existing reverse Styx session. A receiver uses
the transport already associated with the enclosing message. Echoing a
grant on that channel resolves to the original object.

When all Styx request tags are busy, mounted channel I/O and fid
retirement wait for a free tag before sending. Cancellation can stop
the wait, and the session timeout bounds it.

Message grants live within the enclosing result's lease. Revoking the
function fid cancels the module's descendant tokens. Releasing the
destination lease also withdraws its reverse publications. Cycles of
lease dependencies are rejected.

The wire bound is 64 KiB per operation, including its headers and any nested
guards. Channel handles can appear directly or in arrays and tuples of
arguments and results. Each channel scope supports up to eight distinct
channels, including grants made in messages. An `alt` can include up to eight
eligible options after expanding receive arrays. A module can
hold up to 128 active groups, including child tokens. A lease can publish
up to eight distinct reverse grants, subject to the 16-publication limit
shared with active caller contexts on the reverse session.

## Wire records

| Tag | Record |
| --- | --- |
| 24 | Argument or result capability with a positive 32-bit key in network byte order. Tag 9 represents `nil`. |
| 25 | Send with a four-byte key followed by a typed value frame. An `int` channel retains the original scalar record. |
| 26 | Receive with a four-byte key. |
| 27 | Int-only reply using the [UniBus scalar record](../../native/terminal/scalar_record.h). An `alt` reply packs the one-based guard ordinal in the high 32 bits and the value bits in the low 32 bits. Zero denotes the fallback. |
| 28, 29 | Blocking or fallback selection, respectively. The payload is an ordered sequence of send and receive records. |
| 30 | Typed reply. The first byte is the one-based guard ordinal, zero for fallback or 255 for failure. A selected receive includes one managed value frame. |
| 31 | Provider-owned channel in a function reply. Contains a 16-byte group token and a network-order word with a positive 31-bit key. Its high bit marks a zero-capacity channel. |
| 32 | Mixed selection's bounded remote offer. Uses the tag 28 payload and the tag 27 or 30 reply. An unpaired offer replies with ordinal zero at its deadline. |
| 33 | Nested reverse grant. A network-order 32-bit publication identifier followed by the key word from tag 31. |
| 34 | Nested provider-issued key in a channel reply. A positive 31-bit key in a four-byte word. The tag marks a zero-capacity channel. |
| 35 | Nested module grant. A 16-byte token from the same module registry followed by the key word from tag 31. |

Use matching DisVM versions on both ends for tag 31 capacity metadata and
the nested grant records.

[Native transport](../../runtime/vfs/channel_bus.c) ·
[Dis instructions and channel matching](../../runtime/vfs/channel_proxy.c)
