# Leased frame resources

The additive C89 [frontend/common/resource_frame.h](../frontend/common/resource_frame.h) API exports copied logical
stage RGBA frames through the existing dispatcher, S-record envelopes and
resource files. It complements [timeline state](PLAYER_RESOURCE_STATE.md),
[frame observation](FRAME_CAPTURE.md), [9P](RESOURCE_9P.md) and the
[capture example](../examples/player/frame_capture/README.md).
[Русский](PLAYER_RESOURCE_FRAMES.ru.md).

## Ownership and bounds

Create chooses an explicit capacity per pixel slot, from one byte through
[FVM_DESKTOP_RFRAME_MAX_BYTES](../frontend/common/resource_frame.h#L29) (64 MiB). There are [FVM_DESKTOP_RFRAME_SLOTS](../frontend/common/resource_frame.h#L18) (three)
slots: total pixel allocation is three times that capacity, plus fixed native
metadata. All allocations precede resource registration; errors free partial
storage and leave both outputs unchanged. There is no hidden maximum-size
allocation. After create, capture, requests and cleanup allocate nothing.

Capture runs on the frame-observer owner and copies visible RGBA rows to a
nonlatest, unpinned spare. Source padding is discarded. It publishes metadata
before committing the new latest slot. No runner pixel/source/context/root
pointer is retained. Failed geometry/publication leaves the previous published
image intact. The native frame cache and live leases can outlive runner/SDK
teardown as long as their dispatcher remains alive and owner calls are serialized.
[GC:] No managed roots, collection, VM calls or SDL work are introduced.

Each successful capture has a nonzero Uint32 sequence in this adapter lifetime;
exhaustion returns LIMIT rather than wrapping. A client must ACQUIRE a lease
before CHUNK reads. The image remains immutable until that lease is released,
revoked or the adapter is destroyed, even while newer frames arrive. INFO and
metadata events do not create a pin. There are [FVM_DESKTOP_RFRAME_LEASES](../frontend/common/resource_frame.h#L20) (eight)
lease slots, authenticated per session with independent nonwrapping generations.
No fairness, implicit lease timeout, persistent handle or cross-restart identity
is promised. An authenticated reader can consume the bounded pin quota; host
policy must release/revoke lost or abusive sessions.

If old leases leave no spare slot, capture returns BUSY, preserves the current
image/leases/sequence, and increments a saturating dropped counter. The direct
frame-notify wrapper treats BUSY as successful playback: only export is dropped.
INFO/ACQUIRE report the current dropped count; events report it at publication.
At Uint32 max it is a lower bound. This is separate from an observer event's
missed count, which reports that subscriber's queue overflow. Configured capacity
or sequence exhaustion is LIMIT, not a silent drop. Capture never blocks waiting
for readers and never acknowledges network delivery.

## Resource operations

The host chooses the registered path, for example `/display/frame`. The current
filesystem projects it under its incarnation directory, e.g. `/3.1/path` names
the registration and `/3.1/ops/2` invokes ACQUIRE. Discover actual slot/generation
names; do not assume them in applications. All five operations require READ;
SUBSCRIBE can independently grant metadata events. Lease IDs alone grant no
permission, and a wire uname/session claim cannot choose native authority.

| Operation | Request and result |
| --- | --- |
| [FVM_DESKTOP_RFRAME_INFO](../frontend/common/resource_frame.h#L8) | Empty request, return latest metadata with zero/zero lease. Before first capture terminal BUSY. No pixel pin is allocated. |
| [FVM_DESKTOP_RFRAME_ACQUIRE](../frontend/common/resource_frame.h#L9) | Empty request, return latest metadata with a new session-owned lease ID/generation. Full/retired lease table LIMIT. Errors acquire no pin. Repeated calls intentionally allocate independent leases. |
| [FVM_DESKTOP_RFRAME_CHUNK](../frontend/common/resource_frame.h#L11) | Exactly four canonical nonnegative S integers: lease ID, generation, byte offset, count. Count 1..[FVM_R_BYTES](../include/flashvm/rdispatch.h#L9); offset no greater than byte_count. Return up to count raw packed RGBA bytes. At exact end OK/zero bytes; beyond end or invalid count INVALID. Live lease of another session DENIED; stale/revoked/unknown lease UNKNOWN. No word/pixel alignment is required. |
| [FVM_DESKTOP_RFRAME_RELEASE](../frontend/common/resource_frame.h#L14) | Exactly lease ID and generation as two S integers. Release only this session's pin; another owner DENIED, stale handle UNKNOWN. Success has an empty result. Generation exhaustion retires that lease slot permanently. |
| [FVM_DESKTOP_RFRAME_REGION](../frontend/common/resource_frame.h#L30) | READ: lease ID, generation, x, y, width, height. Packed RGBA rectangle, at most 512 bytes; invalid geometry INVALID, oversized reply LIMIT, stale lease UNKNOWN, other session DENIED. |

Every ordinary resource write still queues an operation; its native owner must
pump it. INFO/ACQUIRE/CHUNK/REGION/RELEASE handlers access only native cache storage.
They may be pumped after runner return without touching a dead VM. During a live
player, pump at safe owner boundaries; control operations retain their separate
notify-only delivery contract. Do not pump arbitrary VM handlers from a frame
callback just because these frame handlers happen to be native-only.

A CHUNK reply is already counted bytes inside the existing checked resource
reply envelope. Metadata is seventeen canonical unsigned S integers followed by
source_size bytes: version, lease ID, lease generation, sequence, format, width,
height, row_bytes, byte_count, movie_ticks, total_ticks, presentations,
total_presentations, reason, flags, dropped, source_size. Packed rows are width*4,
top to bottom, straight-alpha R/G/B/A octets. No native endian pixel words or
structs are transmitted. Source prefixes are bytewise bounded to 255 and explicitly
flagged; they need not end on a UTF-8 code-point boundary. Native counters greater
than Uint32 saturate with an explicit flag. Counters describe host ticks/draws,
not script `_currentframe`. These are raw raster frames, not a video codec or
an audio/video clock synchronization implementation.

## C API and lifetime

All adapter mutations and handlers run on the serialized owner; worker clients
only post/take copied dispatcher data using its mutex policy, or decode copied
metadata. Input/output objects must not overlap opaque storage or each other.
The host owns authentication, transport, native callback installation and grants.

| Function | Contract |
| --- | --- |
| [fvm_desktop_resource_frame_create](../frontend/common/resource_frame.h#L70) | Dispatcher, canonical path, per-slot capacity, adapter and target outputs. Allocate fixed native cache and five READ operations; borrow dispatcher until destroy. Invalid descriptor/capacity INVALID, oversize LIMIT, allocation NOMEM, registration errors propagate. Outputs only on OK. No SDK/device startup or grants. |
| [fvm_desktop_resource_frame_capture](../frontend/common/resource_frame.h#L79) | Adapter and borrowed valid frame. Validate format, reason, bounded dimensions, counters, source/visible extents; copy packed pixels to a spare, publish metadata, commit latest. No allocation. Invalid frame INVALID; capacity/sequence LIMIT; missing spare BUSY increments dropped only. Publication failures preserve prior published image. Caller owns callback failure/drop policy. |
| [fvm_desktop_resource_frame_notify](../frontend/common/resource_frame.h#L85) | Standard frame observer callback; userdata is adapter. OK or BUSY returns [FVM_OK](../include/flashvm/flashvm.h#L42), intentionally dropping only bounded export under backpressure. INVALID maps [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); other failures [FVM_ERROR](../include/flashvm/flashvm.h#L43). Retains no callback pointer/source/pixel/VM borrow. A host may instead call capture to choose another policy. |
| [fvm_desktop_resource_frame_read](../frontend/common/resource_frame.h#L89) | Decode one counted metadata payload, not the outer S envelope. Validate complete schema, lease pair/range, packed geometry, flags, nonzero sequence/presentations, reason, source length and exact end. INVALID preserves native output. Client owns the copied result; no adapter/VM access or allocation. |
| [fvm_desktop_resource_frame_revoke](../frontend/common/resource_frame.h#L94) | Owner supplies adapter and authenticated session after transport quiesces. Remove this resource's rights, destroy its watches and release every session-owned pin. Errors leave pins intact; a regrant cannot revive old generations. Also disconnect the native dispatcher session when appropriate. |
| [fvm_desktop_resource_frame_destroy](../frontend/common/resource_frame.h#L100) | Owner quiesces capture/handlers/clients, then unregisters exact resource and frees all native buffers, invalidating even outstanding pins. Running handler BUSY preserves pointer. On OK null pointer; queued old jobs later complete UNKNOWN and still need acknowledgement/disconnect. Dispatcher/client files/SDK remain host-owned. |

Close/release each image explicitly. Closing an operation file, fid or even the
9P adapter does **not** automatically release application-specific frame leases.
A lost ACQUIRE reply can leave a pin until RELEASE or native revoke; retries can
allocate another pin. Transport disconnect cleanup must call frame revoke before
dispatcher disconnect, or destroy the entire frame adapter after callers quiesce.
A plain external grant removal alone denies further reads but does not free pins.
This explicit separation avoids inferring ownership from unauthenticated packets.

## Example and build

`frame_capture --resource-frames` adds a 4-MiB-per-slot budget (12 MiB total pixels),
exports every presented frame and decodes one independent metadata event. It also
enables the existing resource-control/state example; ordinary capture and
`--resource-control` keep their previous memory budget. Over-budget frames fail
explicitly. The example demonstrates a local publisher/metadata reader, not a
network listener or a complete remote GUI. Its ordinary PPM remains a separately
owned copy written after SDK teardown. [Budget header](../examples/player/frame_capture/resource_limits.h).

```sh
cmake --build build-player --target flashvm_frontend_resource_frame flashvm_resource_frame_oracle flashvm_example_frame_capture
ctest --test-dir build-player -R '^flashvm_resource_frame' --output-on-failure
./build-player/frontend/flashvm_example_frame_capture --offscreen --resource-frames --embedded flashVM/examples/player/frame_capture/start.swf frame.ppm
```

Configure the static SDL host as documented in [desktop build](DESKTOP_FRONTEND.md).
The standalone contract checks copied padded rows, immutable old leases, bounded
chunks/EOF, competing sessions, explicit release/revoke, lease/buffer exhaustion,
allocation failures at every buffer allocation, source/counter truncation,
sequence/generation exhaustion and queued stale teardown. A Python integer oracle
checks independent metadata bytes, malformed geometry/leases, all prefixes and
native queue replies/chunk ownership without VM/SDL startup. Tutorial tests
compare every output RGB pixel for file, linked-memory and configured HTTP sources.
All roadmap stages remain open; these checks do not certify every SWF tag,
physical hardware, foreign-platform execution, OS mount or production transport.
## Live 9P pixels v0.737

The live [frame_capture.c](../frontend/tests/frame_capture.c) fixture uses the
actual offscreen player and a test-only C89
[resource_9p_client.c](../frontend/tests/resource_9p_client.c). Sessions 7/8 both
receive frame READ; session 7 separately controls the timeline. Each
ACQUIRE/CHUNK/RELEASE/INFO command travels through standard 9P operation-file
messages with existing tag-zero reply bindings and checked S envelopes. Client
callbacks receive no live pixels/root/context pointer.

At each publication every client first rereads its previous leased frame in
eight 512-byte CHUNK operations and compares all 4096 RGBA bytes with its previous
owned copy. It then RELEASEs, ACQUIREs the latest frame, validates metadata and
compares the complete new image. The two lease IDs differ. Old images remain
immutable across frame changes and whole-root navigation; export sequence matches
presented frames including paused redraw. The separate independent SWF/pixel
oracle verifies actual colors and geometry position.

After runner return both clients reread their final pinned images and INFO
without accessing the disposed context. After closing 9P/FS, native host cleanup
explicitly revokes both sessions, then destroys the frame adapter. Existing
FILE/MEMORY/HTTP, navigation and STOP/callback-failure during active loadVariables
with cancellation/join remain exercised. JIT scenarios require a nonzero native
counter, not merely a JIT option; unavailable backends explicitly SKIP.

```sh
cmake --build build-player --target flashvm_frontend_resource_9p_player
ctest --test-dir build-player -R '^flashvm_resource_9p_player_' --output-on-failure
```

This player fixture uses in-memory transport. It verifies files/messages/native
ownership and real pixels, not a TCP/UDP listener, authentication service or OS
mount. The separate earlier 9P wire fixture covers fragmented TCP loopback.
## v0.736–v0.737 verification

Linux x86-64 passed 48 selected native and 13 shared-host tests without skips.
Native enabled socket HTTP; shared disabled HTTP providers and explicitly checked
capability absence. Runs used offscreen/dummy rather than physical devices.
ASAN/UBSAN passed the allocation/lifecycle contract and new frame/state/protocol/
client modules in the actual player fixture for interpreter and JIT. Underlying
VM/SDL archives were ordinary builds and leak enumeration was disabled; the unit
also accounts for every native allocation/free during failures and duplicate
registration. This is not a full sanitized VM acceptance run.

The C89 frame/lease contract cross-linked for ARM64 Linux musl, RISC-V64 Linux
musl and Windows x64 GNU; C89 9P client and C++98 player-host boundary compiled to
target objects. Foreign binaries were not executed. A standalone module check is:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors \
  -IflashVM/include flashVM/frontend/tests/resource_frame.c \
  flashVM/frontend/common/frame.c flashVM/src/protocol/rdispatch.c \
  flashVM/src/protocol/srecord.c -o frame-contract-arm64
```

The fixture includes actual resource_frame.c with fault injection and needs no
VM/SDL. For the whole player host use a toolchain and static SDL SDK matching the
target OS/CPU: see [desktop build](DESKTOP_FRONTEND.md) and
[platform validation](PLATFORM_VALIDATION.md). Cross-compiling this module does
not replace full player build/execution, Android Activity/surface integration or
device acceptance.
## Frame regions v0.738

`FVM_DESKTOP_RFRAME_REGION` (operation 5, READ) takes exactly six canonical
nonnegative S integers: lease ID, generation, x, y, width, height. Coordinates
are zero-based logical stage pixels, with the origin at the upper left. Width
and height must be positive; the entire rectangle must lie within the leased
image. The reply contains tightly packed row-major RGBA bytes, without source
row padding or an additional header. Its size is exactly width*height*4.

Invalid geometry, trailing/missing fields or noncanonical integers return INVALID.
A valid rectangle exceeding [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) (512) returns LIMIT with no partial
pixels; divide larger rectangles into tiles. Stale leases return UNKNOWN and
another session's lease DENIED, as with CHUNK. Reads do not allocate, release a
pin or change the cached image. This does not calculate dirty rectangles.

For example, `[lease, generation, 7, 5, 16, 8]` returns 512 bytes from eight
rows, each containing sixteen pixels. Every row in the response is 64 bytes;
the full image's row stride is not present in the reply.

The independent Python oracle compares all rectangles of a 2x2 image and 300
seeded rectangles of an 8x32 padded source, including exact 512-byte and
oversized requests. Two live 9P readers also compare an interior 16x8 region
of actual player frames, retained old images and final images after teardown.

9P negotiates a whole-message `msize` in the initial Tversion/Rversion exchange.
It includes 9P headers; the dispatcher payload bound [FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) is independent.
A resource reply file may require multiple Tread/Rread exchanges when the
negotiated msize is smaller than its encoded S-record. Raising msize does not
raise the native dispatcher payload limit. See the [9P adapter](RESOURCE_9P.md).
## Negotiated 9P reads v0.739

The live player fixture now negotiates msize 256 for one reader and 384 for
the other, using Tversion/Rversion before attach. Both values bound the complete
9P message, including its header. Every received response is checked against
that connection's negotiated bound. A 512-byte pixel payload plus its S-record
and resource envelope therefore requires several reads on either connection.
The client accumulates a bounded reply file to EOF and only then decodes its
declaration and complete checked S-record; no partial record is interpreted.

This exercises actual player CHUNK and REGION results through small-message
9P exchanges, while retaining independent leases across presentations and VM
teardown. The fixture's small timeline events fit a single Rread; it is not a
general-purpose incremental event client or production network listener. Native
[FVM_R_BYTES](../include/flashvm/rdispatch.h#L9) remains 512, the server maximum msize remains 4096, and neither
existing public structure layouts nor the metadata version changed.
Verification of v0.738–v0.739: 48 selected native and 13 shared-host tests passed.
New region code and the live 9P fixture passed AddressSanitizer/UBSan with
leak enumeration disabled in this restricted host. The C89 region contract
cross-linked and the sized client cross-compiled for ARM64 Linux, RISC-V64
Linux and Windows x64; foreign binaries were not executed.
Negotiated native bulk export of existing frame leases: [player frame parts](PLAYER_FRAME_PARTS.md),
with [runnable host](../examples/player/frame_parts/README.md). CHUNK/REGION and
their bounded resource replies remain unchanged.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_R_BYTES` | macro / define | [include/flashvm/rdispatch.h:9](../include/flashvm/rdispatch.h#L9) |
<!-- source-links:end -->
