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
| [`fvm_sslot_write`](../include/flashvm/sslot.h#L15) | Stage a checked record and zero padding, then write exactly 1024 bytes. Caller owns output; insufficient capacity/body LIMIT, bad argument INVALID. Output and used unchanged on failure. Payload may contain arbitrary binary bytes or nested records. |
| [`fvm_sslot_read`](../include/flashvm/sslot.h#L21) | Borrow exactly one full slot. Short input MORE, excess input INVALID; malformed record/checksum/padding fails without changing view/used. Embedded MORE in a complete slot becomes INVALID. Successful view is borrowed until caller changes input. |
| [`fvm_sslot_batch`](../include/flashvm/sslot.h#L38) | Walk at most 64 slots, reporting each independently. Invalid extent INVALID, too many slots LIMIT, before any callback. A bounded batch returns OK even with damaged slots; inspect report/notifications. Optional callback cannot stop iteration or mutate the input. Report changes only after traversal. |

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
| [`fvm_smedia_write`](../include/flashvm/smedia.h#L55) | Encode one exact part into one 1024-byte slot; no allocations. Output/used atomic on error. |
| [`fvm_smedia_read`](../include/flashvm/smedia.h#L60) | Validate slot, agreed tag and exact fragment extent; publish a borrowed piece only on OK. |
| [`fvm_smedia_receiver_init`](../include/flashvm/smedia.h#L65) | Copy trusted expected identity, clear bitmap, borrow sufficient caller buffer; never clear buffer bytes. Error preserves previous receiver. |
| [`fvm_smedia_accept`](../include/flashvm/smedia.h#L73) | Verify and copy one new matching part or accept an identical duplicate. Set complete when all parts arrived. All error outputs remain unchanged. |
| [`fvm_smedia_missing`](../include/flashvm/smedia.h#L80) | Enumerate bounded FILE holes from start; continue with last+1. LIVE returns zero. No I/O/timers/requests, count changes only on OK. |

[Runnable tutorial](../examples/embedding/media_parts/README.md) demonstrates a
corrupt middle slot, selective file retry, live omission and compact control.
The independent Python oracle verifies exact bytes, truncated prefixes, metadata,
last bitmap bit, reordered UDP and a small TCP missing-part request. The sockets
belong to the test harness; native modules remain transport-independent.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:73](../include/flashvm/smedia.h#L73) |
| `fvm_smedia_missing` | function | [include/flashvm/smedia.h:80](../include/flashvm/smedia.h#L80) |
| `fvm_smedia_read` | function | [include/flashvm/smedia.h:60](../include/flashvm/smedia.h#L60) |
| `fvm_smedia_receiver_init` | function | [include/flashvm/smedia.h:65](../include/flashvm/smedia.h#L65) |
| `fvm_smedia_write` | function | [include/flashvm/smedia.h:55](../include/flashvm/smedia.h#L55) |
| `fvm_sslot_batch` | function | [include/flashvm/sslot.h:38](../include/flashvm/sslot.h#L38) |
| `fvm_sslot_read` | function | [include/flashvm/sslot.h:21](../include/flashvm/sslot.h#L21) |
| `fvm_sslot_write` | function | [include/flashvm/sslot.h:15](../include/flashvm/sslot.h#L15) |
<!-- source-links:end -->
