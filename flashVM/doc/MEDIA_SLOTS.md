# Fixed slots and media parts

[Русский](MEDIA_SLOTS.ru.md). This additive standalone C89 API prepares binary
S-record batches for file and real-time media channels. It does not change the
S-record profile, resource request schema or standard 9P wire messages.

Small commands, properties, replies and events retain ordinary variable-length
S-records without padding. Fixed extent is not a channel-wide requirement.
The slot profile is explicitly enabled only for bulk objects/media; a small
object may be sent as one ordinary S-record instead.


## Recovery profile 1

Agree the slot profile over an authenticated reliable control channel before
receiving payloads. Profile 1 uses exactly 1024 bytes per slot. The slot begins
with one ordinary S-record (checksum, length, direction-local tag, payload);
all remaining bytes are zero padding. There is no native-endian wrapper or
second length to trust. All S-record numeric fields keep their existing
canonical big-endian signed-magnitude encoding. A tag-zero declaration can be
encoded in a slot, but the slot module never installs bindings implicitly.

A batch concatenates zero through 64 complete slots. A damaged record, including
its length, cannot consume the next slot: advance exactly 1024 bytes from the
known batch start. Validate every record checksum and all padding separately.
A trailing partial slot rejects the whole batch before callbacks; the transport
must accumulate TCP prefixes or discard a truncated UDP datagram first. The
record's encoded_size excludes padding; successful slot used is always 1024.
The batch callback receives NULL instead of a record for a rejected slot.

This preserves alignment only when the transport supplies the correct start
and extent. It does not recover arbitrary byte insertion/deletion in an unknown
stream, correct corrupt data, authenticate the sender, or guarantee detection
of every checksum collision. Existing S-record checksum-width rules remain
unchanged. Do not search for an apparent CRC match to rediscover a lost boundary.

## Native API

All functions allocate nothing, perform no I/O, and require serialized ownership.
Views borrow input storage. Input, output and native state must not overlap.
No VM/SDL pointers, GC roots, worker threads or process-global state are retained.

| API | Ownership, result and errors |
| --- | --- |
| [`fvm_sslot_write`](../include/flashvm/sslot.h#L17) | Stage a checked record and zero padding, then write exactly 1024 bytes. Caller owns output; insufficient capacity/body LIMIT, bad argument INVALID. Output and used unchanged on failure. Payload may contain arbitrary binary bytes or nested records. |
| [`fvm_sslot_read`](../include/flashvm/sslot.h#L23) | Borrow exactly one full slot. Short input MORE, excess input INVALID; malformed record/checksum/padding fails without changing view/used. Embedded MORE in a complete slot becomes INVALID. Successful view is borrowed until caller changes input. |
| [`fvm_sslot_batch`](../include/flashvm/sslot.h#L40) | Walk at most 64 slots, reporting each independently. Invalid extent INVALID, too many slots LIMIT, before any callback. A bounded batch returns OK even with damaged slots; inspect report/notifications. Optional callback cannot stop iteration or mutate the input. Report changes only after traversal. |

## Transport boundaries

9P's initial Tversion/Rversion negotiates msize for a whole 9P message, including
headers. File data may contain multiple S-records, or portions of a larger
application object. Slot boundaries belong to that file's agreed payload
profile; they are not additional 9P requests. A large stream read may split a
slot and must be accumulated. The existing operation-file adapter still accepts
one request of at most [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9), not arbitrary media writes: this module is
not a hidden extension of that endpoint.

For TCP control/file channels, a host may batch slots up to negotiated limits.
For UDP immediate media, choose a datagram extent that fits the validated path
MTU and pacing policy; profile 1 permits one 1024-byte slot per datagram. Larger
batches on a LAN require explicit transport policy, not an assumption that a
64-KiB batch fits the Internet path. UDP loss/reordering and corruption must be
handled at the object layer. Standard UDP checksum rejection may discard an
entire datagram before this code sees it. S-record CRC is not cryptographic
integrity; authenticate the transport/session separately. Control, tag binding,
retransmission requests and rate changes remain on reliable authenticated TCP.

Primary specifications: [9P version negotiation](https://9fans.github.io/plan9port/man/man9/version.html)
and [RFC 8085 UDP guidance](https://www.rfc-editor.org/info/rfc8085/).

## Building and verification

`flashvm_srecord` and the core SDK include sslot.c; no SDL is required for the
standalone target. Configure as described in [building](BUILD_PLATFORMS.md), then:

```sh
cmake --build build --target flashvm_srecord_oracle
ctest --test-dir build -R flashvm_srecord_slot_oracle --output-on-failure
```

The independent Python big-integer/zlib oracle checks encoded bytes, every
truncated prefix, atomic output, all-padding corruption, damaged length isolation,
400 seeded mutations, multi-slot batches and real TCP/UDP IPv4 loopback sockets.
The loopback sockets are a test harness, not an installed player listener.
## File and live fragments v0.741

`smedia.h` defines the optional `flashvm.media.fragment` type. Bind a nonzero tag
separately for each direction on an authenticated reliable channel. A fragment
payload contains eight canonical nonnegative S integers in this order:
`version=1, mode, stream, epoch, object, time_ms, total, part`, then raw bytes.
Mode is FILE=1 or LIVE=2. Stream, epoch and object are nonzero Uint32; time_ms is
Uint32 relative presentation metadata interpreted by the host, not wall time or
an automatic scheduler. Reconnection/reuse requires a new epoch agreed with the
peer; the module does not generate identities or wrap them automatically.

Nonfinal pieces contain exactly 768 bytes, the final piece the exact remainder.
Offset is derived as part*768, with no peer-selected overlapping offsets. Total
is 1..3145728 bytes and at most 4096 parts; the bitmap is 512 bytes. This bounds
one object, not an entire file/connection: larger files or videos use multiple
objects with separate host-managed ordering/completion. An empty file is handled
by the ordinary control/file protocol, not a zero-part bulk object. Fixed profile
sizes are versioned opt-in choices, not negotiated by untrusted data packets.

Receiver initialization copies the expected identity and borrows a caller buffer
of at least total bytes. It never adopts the first packet's metadata. A received
slot is checksum/format/tag/identity checked before copying. Unchanged duplicate
parts succeed idempotently; different valid bytes for an accepted part or a
changed identity return CONFLICT. Wrong tag UNKNOWN; malformed metadata INVALID;
short slot MORE; capacity LIMIT. Errors leave buffer, receiver and complete
unchanged. Only accepted parts are initialized: holes contain arbitrary prior
bytes, and the whole object is usable only when complete is true. Part views
from read borrow the input and can instead feed a host's immediate consumer.
There is no whole-file digest or cryptographic authenticity guarantee.

FILE missing enumerates absent indices in ascending pages. Send bounded/rate-
limited requests over reliable authenticated TCP and keep sender storage until
acknowledged completion. A correct retry has the same identity, part index and
bytes. LIVE missing always returns zero: the host skips losses at its deadline,
performs codec concealment if supported and explicitly reinitializes for the
next object. Out-of-order parts within the current object are accepted; old
objects/epochs are rejected, with no automatic eviction or deadline extension.
This implements reassembly/retry policy, not codecs, audio/video synchronization,
STUN, pacing, UDP authentication, a network listener or player media playback.

| API | Contract |
| --- | --- |
| [`fvm_smedia_write`](../include/flashvm/smedia.h#L64) | Encode one exact part into one 1024-byte slot; no allocations. Output/used atomic on error. |
| [`fvm_smedia_read`](../include/flashvm/smedia.h#L69) | Validate slot, agreed tag and exact fragment extent; publish a borrowed piece only on OK. |
| [`fvm_smedia_receiver_init`](../include/flashvm/smedia.h#L74) | Copy trusted expected identity, clear bitmap, borrow sufficient caller buffer; never clear buffer bytes. Error preserves previous receiver. |
| [`fvm_smedia_accept`](../include/flashvm/smedia.h#L82) | Verify and copy one new matching part or accept an identical duplicate. Set complete when all parts arrived. All error outputs remain unchanged. |
| [`fvm_smedia_missing`](../include/flashvm/smedia.h#L89) | Enumerate bounded FILE holes from start; continue with last+1. LIVE returns zero. No I/O/timers/requests, count changes only on OK. |

[Runnable tutorial](../examples/embedding/media_parts/README.md) demonstrates a
corrupt middle slot, selective file retry, live omission and compact control.
The independent Python oracle verifies exact bytes, truncated prefixes, metadata,
last bitmap bit, reordered UDP and a small TCP missing-part request. The sockets
belong to the test harness; native modules remain transport-independent.
## Tag-zero size agreement v0.742

Tag 0 is multifunctional. Legacy first-payload-integer 1 still identifies the
existing immutable tag-binding schema; it is not reinterpreted. First integer
2 identifies the new service envelope. The compact service payload contains
exactly eight canonical nonnegative S integers:
`2, command, id, tag, record_limit, slot_bytes, part_bytes, reserved=0`.
Command 1 is OFFER, 2 ACCEPT. Unknown service versions/commands return UNKNOWN
without outputs; a host can dispatch known services explicitly. Do not pass
unknown services to the bindings registry or silently treat them as permission.
Additional service commands can be introduced separately in the future.

Tag identifies one already bound type in one direction. Record_limit bounds
encoded tag+payload bytes, not the outer checksum/length or an entire 9P packet.
Zero slot_bytes and part_bytes selects ordinary compact records. Fixed bulk
slots are 256..65536 bytes with record_limit <= slot_bytes-32, record_limit >64,
and 1 <= part_bytes <= record_limit-64. These conservative reserves keep headers
inside the agreed buffer. Native size_t must represent the selected capacity.
The local ceiling advertises real host storage/quota; it does not allocate it.
A fixed-capable ceiling may also negotiate compact records. Separate directions
may bind different tags/types/sizes and must use separate exchange state.

State transitions are explicit: init has no active framing; propose creates an
inactive OFFER. Respond chooses limits no greater than either peer's limits,
returns ACCEPT and keeps them pending. The responder commits its exact selection
only after its authenticated reliable transport successfully queues the ACK;
the initiator commits only a correlated bounded ACK. Exact committed ACK repeats
are idempotent. Wrong tag/ID, enlarged ACK and competing negotiation CONFLICT;
malformed metadata INVALID. Active values remain unchanged on all failures.
IDs increase through Uint32 without reuse; at exhaustion create a fresh session.
Cancel preserves active values and retires the pending ID; a late cancelled ACK
cannot commit. An uncertain peer state after control disconnect must be reconciled
or replaced with a new session before payload resumes. This is not distributed
consensus or cryptographic authentication.

Quiesce old payload traffic before changing framing. A host applies active limits
when validating the corresponding file/data stream: the generic fixed-slot reader
checks slot boundaries, not an arbitrary per-tag record quota. Use the agreed
record_limit with [fvm_srecord_read](../include/flashvm/srecord.h#L80) before dispatching general record payloads.
Do not assume 9P msize changes the application record quota or socket datagram
size. Service records themselves remain small variable-length S-records.

| API | Execution and ownership |
| --- | --- |
| [`fvm_scontrol_write`](../include/flashvm/scontrol.h#L67) | Serialize a compact checked service record; caller output/used unchanged on error. |
| [`fvm_scontrol_read`](../include/flashvm/scontrol.h#L69) | Verify one record and copy service metadata; borrowed input ends on return. Trailing records remain unconsumed; UNKNOWN preserves outputs. |
| [`fvm_scontrol_init`](../include/flashvm/scontrol.h#L74) | Copy local maxima, clear pending/active state; no allocations. |
| [`fvm_scontrol_propose`](../include/flashvm/scontrol.h#L78) | Store a single pending monotonically identified offer within local quotas; no active change or transport call. |
| [`fvm_scontrol_respond`](../include/flashvm/scontrol.h#L83) | Select smaller offer/local limits, store pending ACK, publish copied reply only on success. |
| [`fvm_scontrol_commit`](../include/flashvm/scontrol.h#L90) | Check correlation and bounded/exact ACK, then replace active values atomically. Host owns reliable queueing and payload quiescence. |
| [`fvm_scontrol_cancel`](../include/flashvm/scontrol.h#L95) | Discard pending state, preserve committed values and ID history. No remote rollback. |
| [`fvm_sslot_write_sized`](../include/flashvm/sslot.h#L45) | Encode one checked padded record at an already agreed slot extent, without slot-sized scratch or allocation. Atomic output/used on error. |
| [`fvm_sslot_read_sized`](../include/flashvm/sslot.h#L48) | Check the configured full slot and zero padding; borrow one verified record only on OK. |
| [`fvm_sslot_batch_sized`](../include/flashvm/sslot.h#L50) | Traverse up to 64 configured slots independently; never derive the next boundary from damaged contents. |
| [`fvm_srecord_write_pair`](../include/flashvm/srecord.h#L72) | Encode two borrowed byte spans as one ordinary record, avoiding an object-sized concatenation buffer; same bytes/CRC as write of their concatenation. Atomic validation/capacity errors. |
| [`fvm_smedia_write_profile`](../include/flashvm/smedia.h#L94) | Use agreed slot/part geometry, default fragment schema and bounded object identity; write with header+data scatter input. |
| [`fvm_smedia_read_profile`](../include/flashvm/smedia.h#L98) | Use already agreed geometry to verify exact typed part extent and expose borrowed bytes. Packet metadata cannot change geometry. |
| [`fvm_smedia_receiver_init_profile`](../include/flashvm/smedia.h#L100) | Copy agreed profile and trusted identity; borrow sufficient object buffer. Old/default entry points retain the opt-in 1024/768 profile. |

Part geometry is copied into the receiver; changing it requires quiescence and
an explicitly initialized new object. Total remains <=3 MiB and parts <=4096,
even with large negotiated slots. Count/offset/final remainder use agreed
part_bytes, not the convenience default 768. The native receiver structure gained
a profile in this development series; use headers and library from the same
build and initialize through the API, never a serialized native layout.

The tutorial now performs a compact tag-zero handshake before bulk transfer.
The independent control oracle negotiates 65536 down to 4096/4000 and reconstructs
actual differently sized media parts, tests compact mode, cancelled/stale ACKs,
atomic rejections and sized slots through 64 KiB. A native C89 contract exercises
the same handshake and large-slot path for sanitizers and cross-linking.
## Change sizes during a live transfer

Boundary-aware commands OFFER_AT=3 and ACCEPT_AT=4 append three canonical
Uint32 fields after the eight generic fields: `stream, epoch, first_object`.
All three must be nonzero; ACK must echo them exactly. Generic commands 1/2
retain their eight-field format and require host-quiesced tag traffic.
[`fvm_scontrol_propose_at`](../include/flashvm/scontrol.h#L101) requests either a smaller or larger profile, bounded
by both peers' unchanged capacity ceilings. A new increasing request ID is
required even when returning to an earlier buffer size.

Only later objects use the committed new profile. Finish or retire the previous
object with its previous receiver; do not reinterpret its partial buffer or
already accepted parts using a different part size. The host selects a future
object boundary based on its live segment counter. It need not pause the whole
player timeline. [`fvm_smedia_receiver_init_agreed`](../include/flashvm/smedia.h#L108) copies committed geometry and
rejects another stream/epoch or an object before first_object. Late old-profile
UDP packets fail extent or expected-identity checks without modifying new object
storage. If old and new profiles coexist temporarily, retain separately bounded
old/new receivers and route only authenticated known object identities. This
module does not automatically allocate such receivers or drain sockets.

Higher loss may motivate a smaller slot; stable low-loss/low-latency intervals
may justify growth within path-MTU and host limits. Neither change is automatic:
loss may have causes other than packet size. The host must use bounded observation
windows, hysteresis/cooldown and pacing rather than renegotiating on every loss.
No congestion controller, packet-loss estimator or latency probe is claimed here.
Control messages and ACKs remain reliable/authenticated TCP; live data may use UDP.
A 64-KiB application slot is not an instruction to send a 64-KiB UDP datagram.

| API | Contract |
| --- | --- |
| [`fvm_scontrol_propose_at`](../include/flashvm/scontrol.h#L101) | Pending boundary-aware offer, with increasing ID and unchanged active profile. Validates complete boundary; ACK cannot move it. |
| [`fvm_smedia_receiver_init_agreed`](../include/flashvm/smedia.h#L108) | Borrow object storage only after committed noncompact limits exist. Copy agreed profile; wrong stream/epoch/old object CONFLICT before mutation. |

The independent oracle and native contract run 4096/4000 -> 512/400 ->
4096/4000 across successive live objects, reject shifted ACK boundaries and old
objects, and reconstruct every object without mixing sizes. The oracle also
injects late packets from the preceding profile. These are transport/framing
contracts, not automatic media playback or adaptation by the desktop player.
## Verification of v0.740–v0.742

53 selected native contracts passed: 52 in the first run and the existing heartbeat
tutorial after restoring its host-lost executable permission. All 13 shared-host
contracts passed. The new standalone C89 contract and annotated tutorial passed
AddressSanitizer/UBSan with leak enumeration disabled by this restricted host.
The four protocol implementations were instrumented; this is not whole-VM
memory-safety certification. Independent Python oracles use their own integer
and zlib wire generation and actual IPv4 loopback TCP/UDP sockets.

C89 contract and tutorial cross-linked for ARM64 Linux, RISC-V64 Linux and Windows
x64; the public-header consumer compiled as C++98 for all three. Foreign binaries
were not executed. Whole native-window/audio-device platform acceptance remains
open. To reproduce the standalone cross-link from the repository root:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors -IflashVM/include \
  flashVM/tests/protocol/scontrol_contract.c flashVM/src/protocol/srecord.c \
  flashVM/src/protocol/sslot.c flashVM/src/protocol/smedia.c \
  flashVM/src/protocol/scontrol.c -o media-contract-arm64
```

Substitute riscv64-linux-musl or x86_64-windows-gnu for the other checked targets.
The full host build still needs its target static SDL SDK and platform adapter;
see [platform builds](BUILD_PLATFORMS.md) and [desktop host](DESKTOP_FRONTEND.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `fvm_scontrol_cancel` | function | [include/flashvm/scontrol.h:95](../include/flashvm/scontrol.h#L95) |
| `fvm_scontrol_commit` | function | [include/flashvm/scontrol.h:90](../include/flashvm/scontrol.h#L90) |
| `fvm_scontrol_init` | function | [include/flashvm/scontrol.h:74](../include/flashvm/scontrol.h#L74) |
| `fvm_scontrol_propose` | function | [include/flashvm/scontrol.h:78](../include/flashvm/scontrol.h#L78) |
| `fvm_scontrol_propose_at` | function | [include/flashvm/scontrol.h:101](../include/flashvm/scontrol.h#L101) |
| `fvm_scontrol_read` | function | [include/flashvm/scontrol.h:69](../include/flashvm/scontrol.h#L69) |
| `fvm_scontrol_respond` | function | [include/flashvm/scontrol.h:83](../include/flashvm/scontrol.h#L83) |
| `fvm_scontrol_write` | function | [include/flashvm/scontrol.h:67](../include/flashvm/scontrol.h#L67) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:82](../include/flashvm/smedia.h#L82) |
| `fvm_smedia_missing` | function | [include/flashvm/smedia.h:89](../include/flashvm/smedia.h#L89) |
| `fvm_smedia_read` | function | [include/flashvm/smedia.h:69](../include/flashvm/smedia.h#L69) |
| `fvm_smedia_read_profile` | function | [include/flashvm/smedia.h:98](../include/flashvm/smedia.h#L98) |
| `fvm_smedia_receiver_init` | function | [include/flashvm/smedia.h:74](../include/flashvm/smedia.h#L74) |
| `fvm_smedia_receiver_init_agreed` | function | [include/flashvm/smedia.h:108](../include/flashvm/smedia.h#L108) |
| `fvm_smedia_receiver_init_profile` | function | [include/flashvm/smedia.h:100](../include/flashvm/smedia.h#L100) |
| `fvm_smedia_write` | function | [include/flashvm/smedia.h:64](../include/flashvm/smedia.h#L64) |
| `fvm_smedia_write_profile` | function | [include/flashvm/smedia.h:94](../include/flashvm/smedia.h#L94) |
| `fvm_srecord_read` | function | [include/flashvm/srecord.h:80](../include/flashvm/srecord.h#L80) |
| `fvm_srecord_write_pair` | function | [include/flashvm/srecord.h:72](../include/flashvm/srecord.h#L72) |
| `fvm_sslot_batch` | function | [include/flashvm/sslot.h:40](../include/flashvm/sslot.h#L40) |
| `fvm_sslot_batch_sized` | function | [include/flashvm/sslot.h:50](../include/flashvm/sslot.h#L50) |
| `fvm_sslot_read` | function | [include/flashvm/sslot.h:23](../include/flashvm/sslot.h#L23) |
| `fvm_sslot_read_sized` | function | [include/flashvm/sslot.h:48](../include/flashvm/sslot.h#L48) |
| `fvm_sslot_write` | function | [include/flashvm/sslot.h:17](../include/flashvm/sslot.h#L17) |
| `fvm_sslot_write_sized` | function | [include/flashvm/sslot.h:45](../include/flashvm/sslot.h#L45) |
<!-- source-links:end -->
