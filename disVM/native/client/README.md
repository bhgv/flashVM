# Native 9P client / Native 9P-клиент

Runtime0.145 adds a small native client and [ordinary VFS mount provider](../vfs/MOUNT.md).
It uses the complete imported Inferno lib9 `convS2M`, `convM2S`, `statcheck` and
`convM2D`. It does not introduce a second wire codec or import the Inferno kernel,
its scheduler or graphics. The original C libstyx servers remain the peer in the
TCP acceptance tests. This host adapter is separate from Inferno's kernel devmnt.

The native host supplies an already connected `StyxTransport`. All four callbacks
must exist: begin/read/write/close; state is their native context. `begin` establishes one
total RPC deadline before any frame bytes are sent; fragments must consume that
same budget. Read/write return actual bounded positive counts, zero on stream
EOF or negative on error. The callbacks can use native OS/network adapters, never
managed callbacks, guest execution or guest suspension. Native I/O may block;
call this API on an authorized native I/O worker. The host chooses transport,
authentication and endpoint access; a wire uname/aname is not a credential.

`styxclientmount` consumes a valid callback transport on every outcome, including
bad options/OOM/negotiation/attach failure. An invalid callback struct remains
caller-owned. It negotiates base9P2000 at256..65536 message bytes, attaches with
NOFID, and returns one owned connection reference. There is no implicit dial,
Tauth exchange, reconnect or retry. `styxmountprovider` subsequently consumes that
reference. Live independent fids retain the connection after its owner drops it.

The connection mutex serializes one complete RPC. Tags are NOTAG for version
and1 for ordinary single-flight RPCs, reused only after their response. No VM,
namespace or global exclusive lock is involved. Two independent native threads
can use different fids; one fid's caller serializes its operations and retirement.
This callback-transport profile stays single-flight. Runtime0.146 provides
multiple tags and Tflush through the explicit fd-owner constructor below; failure/timeout after a frame starts
marks the session terminal instead of allowing an old response to match a new RPC.

Walk validates a canonical relative path before effects; dot clones the root,
absolute/empty/dotdot/embedded-dot components refuse. It batches at MAXWELEM,
clunks partial walks, reserves monotonically allocated32-bit fids without reuse,
and refuses impossible Rwalk counts. Each open has its own remote fid even for
identical Qids. Topen0/1/2 has no create/truncate flags and enforces Ropen iounit
and the negotiated frame bound. Offsets retain the full signed64-bit range.

Every data call issues at most one Tread/Twrite and preserves its actual count.
A short success is not automatically retried and is not EOF. A zero Rwrite for a
positive SDK request becomes NO_PROGRESS. A failed RPC cannot acknowledge bytes;
an attempted data operation without a count, including Rerror, conservatively
has unknown stream/device effects. Its fid rejects further data operations.
Tstat can still diagnose a usable session; the SDK performs that only after
failure. `StyxTransfer.success/count/confirmed/uncertain` are defined by this
adapter; its `stop` field is reserved zero. The VFS bridge supplies SDK stop causes.

Framing/codec/tag/type/count violations and transport failure make the connection
terminal. No later request revives it or automatically replays effects. The
host's read/write callbacks must preserve partial byte counts; the client does
not guess how much an errored callback sent. Successful copied metadata/data
never borrows the next frame's receive buffer.

`styxclientclunk` consumes one fid with a serialized Tclunk when the stream is
usable. It may perform bounded native I/O. Final `styxclientfree` performs no RPC:
it invokes the nonwaiting consuming transport close exactly once, and the server
retires the attached root on disconnect. Thus final provider cleanup need not
wait for a remote acknowledgement. The host close callback must not implement
protocol waits, guest calls or linger that violates this cleanup contract.

Native-клиент использует исходные кодеки Inferno и отдельные remote fid. На одном
соединении пока один RPC одновременно; native I/O worker может ждать bounded I/O,
но API не запускает и не приостанавливает guest-поток. Timeout/потеря ответа не
вызывают reconnect или повтор effects. Мультиплексирование/Tflush добавлены в runtime0.146 ниже; Tauth, typed
remote errors и heartbeat остаются открытыми расширениями.

## Native fd owner (runtime0.146) / Native fd-owner

[MULTIPLEX.md](MULTIPLEX.md) adds an explicit connected-fd constructor with
independent native ownership, concurrent tag-routed RPCs, bounded admission,
cancellation/Tflush and autonomous nonwaiting final retirement. The original
callback transport above keeps its serialized policy. Rebuild native consumers
for the new `StyxTransfer.cancelled` field; wire9P is unchanged.


## Native terminal events / Native terminal events (runtime0.147)

[Automatic connection notifications](EVENTS.md) now connect idle fd-owner EOF and
terminal failures to each mounted provider journal without another RPC or guest
execution. Native subscriptions retain journals independently; unmount does not
wait for callbacks. Shared clients keep independent mount subscriptions. Explicit
shutdown differs from network failure; normal final release is quiet. Heartbeat
and provider-side content/rights/load reports remain separate extensions.


## Native heartbeat / Native heartbeat (runtime0.148)

[Standard Styx heartbeat policy](HEARTBEAT.md) optionally detects silent connections
using a reserved no-fid Tflush/Rflush exchange. Native state snapshots distinguish
send stalls, missed replies, recovery and terminal policy timeout. Each mounted
provider observes transitions automatically through its existing journal, without
guest execution or wire-format extensions. An admitted probe retains its tags and
captured deadlines through disable/reconfiguration. Pending reads remain independent.


## Deferred byte reads / Отложенные byte reads (runtime0.149)

[Original C libstyx deferred read API](../vfs/READ.md) retains ordinary Tread until a
native producer wakes its socket owner. Other clients and heartbeat requests
continue; flush/reset/disconnect/end safely retire pending requests. One-record
TCP fixtures preserve queued data on short buffers. Actual native client tests
combine pending reads, automatic heartbeat and explicit cancellation. Wire Styx
is unchanged; the public provider event format remains undecided.


## Provider event resource / Ресурс событий провайдера (runtime0.150)

[Native event-resource API and examples](../vfs/EVENTS.md) expose a journal as an ordinary
readonly VFS file with explicit native access filtering and host-selected encoding.
One read returns one record; readiness integrates with deferred Styx reads without
guest execution. Cached heads survive short buffers and recheck access at delivery.
Queues are bounded, overflow broadens invalidation, and finish drains before EOF.
No default public wire format/global path is installed; remote client decoding
and subscription discovery remain separate integration work.


## Remote native event feed / Удалённая native-подписка (runtime0.151)

[Configured remote event feeds](../vfs/FEED.md) attach a host-selected endpoint/decoder
to an ordinary mount. A native worker holds a cancellable long-lived read and
publishes into the mount journal without guest progress. Health and remote source
generations are independently rebased with atomic local append. Watch admission
keeps ordinary RPC capacity; provider destruction stops without waiting for Rflush
or decoder return. Discovery and a standard record codec remain separate work.


## Cancellable metadata and walks / Отменяемые metadata и walk (runtime0.156)

[Control cancellation API and example](CONTROL-CANCEL.md) covers native
walk/walkfrom/open/stat with explicit single-use tokens. Long walks retain one
logical token across MAXWELEM fragments; failed walk fids retire independently.
Unacknowledged sent Topen poisons later open/data use, while failed Tstat leaves
its output and fid intact. Original codecs and wire messages are unchanged.

## Carrier boundary / Граница физического транспорта

[`remote_phy`](../../../remote_phy/README.md) now supplies read/write/wait/wake,
availability, connection setup and ownership through native callbacks without an
OS-descriptor requirement. `styxclientmountphy` uses the existing multiplex/flush/
heartbeat engine; `styxclientmountfd` adapts an already granted reliable stream fd.
The fd path supports non-socket byte devices as well as sockets. Raw packet buses
require suitable stream adaptation. Generic reverse RPC and symmetric logical
9P sessions remain separate proposals, described in the carrier guide.

The [original-core PHY server backend](../server/phy/README.md) now supplies the
matching server-side carrier boundary, native producer wake and deferred-read
retirement. Both native roles are tested together over a nonsocket PTY.
