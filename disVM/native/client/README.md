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
