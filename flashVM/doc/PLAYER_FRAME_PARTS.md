# Negotiated parts of actual player frames

[Русский](PLAYER_FRAME_PARTS.ru.md). This owner-only C89 bridge joins the
[copied frame resource](PLAYER_RESOURCE_FRAMES.md) and the
[tag-zero/bulk media protocol](MEDIA_SLOTS.md). It exports actual retained
RGBA pixels using an authenticated session's existing lease and a committed
boundary-aware geometry agreement. No new console dependency or resource wire
operation is introduced.

## Native API and ownership

[`fvm_desktop_resource_frame_write_part`](../frontend/common/resource_frame.h#L110) accepts the frame adapter, trusted
session, lease, committed control exchange, FILE/LIVE policy, relative Uint32
time_ms, part index, caller output/capacity and used. It checks current READ
authority for the frame resource and the exact lease generation/owner before
reading pixels. A numeric lease alone is not authority. Host serializes calls
on the cache owner; do not call concurrently with capture, revoke or destroy.

The agreement must have a nonzero stream/epoch/first-object activation boundary.
The fragment's object ID is the frame cache's nonwrapping capture sequence;
stream/epoch and direction-bound tag come from the agreement. Total is the exact
packed RGBA extent. Part offsets and final length follow agreed part_bytes;
every encoded output occupies agreed slot_bytes. Padding and fragment metadata
follow the existing S protocol. These are bytewise R/G/B/A values, not native
pixel words, compressed images or encoded video.

[`fvm_scontrol_active`](../include/flashvm/scontrol.h#L132) copies validated committed limits and boundary; pending
offers cannot silently change the exporter. It returns UNKNOWN before the first
agreement and preserves both outputs on failure. Adapters need not inspect
mutable exchange fields directly. The exchange remains caller-owned and
serialized, with no transport or implicit authentication.

A new slot is encoded directly from the immutable cached pixel range using
header/data scatter input. No second whole-image copy, allocation, VM/SDL call,
callback, dispatcher pump or socket/file write occurs. The caller owns disjoint
output storage. used and output remain unchanged on any error. The source image
was already copied by capture, so export may outlive the runner/SDK while the
lease, adapter and dispatcher remain alive. Encoding is not delivery or release:
release/revoke every pin explicitly after the consumer no longer needs retries.

| Result | Meaning |
| --- | --- |
| OK | Exactly one complete padded part was written. |
| DENIED | Session lacks current READ authority or owns another lease. |
| UNKNOWN | Lease is stale/released/unknown, or resource disappeared. |
| BUSY | No committed agreement exists yet. A pending offer alone is insufficient. |
| CONFLICT | The retained frame precedes the agreement's first-object boundary. Retain the old agreement for old frames. |
| LIMIT | Caller output too small, image exceeds the bounded media-object size, or configured part size needs too many pieces. |
| INVALID | Invalid native descriptor, compact/non-boundary agreement, mode, time or part index. |

The current per-object quota is 3 MiB and at most 4096 parts. Larger retained
frames remain readable through CHUNK/REGION but cannot be silently truncated or
scaled by this API. A future multi-object image/container layer can split them
explicitly. Geometry, source and tick information remain in ACQUIRE metadata;
a pixel part does not independently identify dimensions or color space. Keep
that metadata with the object's identity.

## Profile changes and transports

For a new capture sequence, agree a profile using OFFER_AT/ACCEPT_AT and that
sequence as first_object. Keep the previous agreement/lease while retrying an
old frame; never reinterpret its data with the new part size. A new adapter or
reused stream requires a new host epoch. Increasing or shrinking slot size does
not change capture sequence, timeline state or native rendering.

The resulting slots are caller-owned bytes. A host can batch several for a
reliable file/TCP channel, or send appropriately sized slots through a paced,
authenticated UDP path. Negotiated msize, path MTU and record quotas are separate.
The existing operation-file request/reply quota remains 512 bytes; this API is
an explicit native bulk adapter, not an unannounced larger 9P operation response.
No automatic congestion adaptation, network listener, socket queue, video codec,
audio/video synchronization or remote display window is installed.

## Validation and building

The C contract uses padded source rows, copies then overwrites original pixels,
checks exact exported bytes, no new native allocations, insufficient capacity,
unknown agreement, another session, stale pin and a future activation boundary.
The independent Python integer/zlib oracle constructs the exact expected 256-byte
slot from original packed pixels and verifies atomic failures.

Build the optional host with its matching static SDL SDK as documented in
[desktop builds](DESKTOP_FRONTEND.md). Relevant targets are
flashvm_player_host, flashvm_frontend_resource_frame and
flashvm_resource_frame_oracle. The frame oracle compiles the detached protocol C modules directly so its C
symbols are available to DLL/shared-object clients; it performs no VM/device startup.
## Real player and transport evidence v0.744

The [annotated frame_parts host](../examples/player/frame_parts/README.md) runs
three real linked-SWF frames offscreen and emits compact type declarations,
frame metadata and OFFER_AT/ACCEPT_AT records before the negotiated data slots.
It exercises 1024/768 -> 512/400 -> 4096/4000, then repeats a retained-slot export
after SDK shutdown. FILE and LIVE output policies are separately selectable.
The library stays independent of its replaceable file adapter and CLI.

An independent integer/zlib Python reader verifies exact wire bytes, identities,
zero padding and every RGBA byte against the authored SWF squares. Actual exported
slots are then sent via IPv4 UDP loopback with reordering, duplicates, a wholly
damaged slot and an omitted slot. The native receiver lists FILE holes; only those
parts are requested/repeated over TCP. LIVE never requests repair. These sockets
are test harnesses, not a production network service or automatic path-MTU policy.

Two live 9P reader fixtures also export their own authenticated leases using the
native bulk API. Their independent direction exchanges shrink/grow between images;
old frames are reassembled while the next image is already published, and final
frames after runner teardown. Interpreter and actual native JIT, local files,
linked memory, HTTP root navigation and callback-error/STOP cancellation retain
their existing pixel and lifetime checks. 9P msize remains independently 256/384;
the larger bulk slots are explicit host output, not oversized operation replies.

54 selected native checks passed (one old executable needed its host-lost execute
bit restored and then passed); all 14 shared-host checks passed. The native frame
contract, changed protocol/cache/client modules with the real interpreter/JIT
player, and the full example passed AddressSanitizer/UBSan. Leak enumeration is
disabled by this restricted host; the complete VM/SDL is not instrumented, so no
whole-program memory-safety claim is made.

The C89 frame contract cross-linked and the tutorial C89/public-header C++98
consumers compiled for ARM64 Linux, RISC-V64 Linux and Windows x64. Foreign
binaries were not executed and the complete foreign player was not linked.
Standalone contract cross-link from repository root:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors -IflashVM/include \
  flashVM/frontend/tests/resource_frame.c flashVM/frontend/common/frame.c \
  flashVM/src/protocol/srecord.c flashVM/src/protocol/rdispatch.c \
  flashVM/src/protocol/sslot.c flashVM/src/protocol/scontrol.c \
  flashVM/src/protocol/smedia.c -o frame-parts-contract-arm64
```

Substitute riscv64-linux-musl or x86_64-windows-gnu for other checked targets.
The whole host needs its target static SDL SDK and platform adapter; see
[platform builds](BUILD_PLATFORMS.md). Native device/platform acceptance and all
nine roadmap stages remain open.
Windows x64 also cross-linked the standalone C89 oracle DLL; its export table
contains the new frame writer, committed-profile query and native receiver APIs.
The DLL was not executed. Evidence sources: [native frame contract](../frontend/tests/resource_frame.c),
[independent exported-frame reader](../frontend/tests/frame_parts_example.py),
[live player fixture](../frontend/tests/frame_capture.c).

See also / См. также: [v0.746 feedback-driven replay](CHANNEL_ADAPTATION.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_scontrol_active` | function | [include/flashvm/scontrol.h:132](../include/flashvm/scontrol.h#L132) |
<!-- source-links:end -->
