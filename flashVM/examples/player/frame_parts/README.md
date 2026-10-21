# Real player frames as negotiated S parts

[Русский](README.ru.md), [contract](../../../doc/PLAYER_FRAME_PARTS.md),
[catalog](../../README.md#example-player-frame_parts).
[main.c](main.c), [limits.h](limits.h). Shared [linked SWF bytes](../frame_capture/movie.inc)
are produced by [make_movie.py](../frame_capture/make_movie.py); this is actual
SWF8 execution, not a replacement renderer. Odd ticks draw a blue left square,
even ticks an orange right square. The actual offscreen player executes three frames.

The host first emits a compact tag-zero declaration for two direction-local types.
For each image ACQUIRE returns immutable lease/geometry metadata; separate tag-zero
messages agree 1024/768, 512/400, then 4096/4000 geometry. The exporter encodes actual
packed RGBA ranges, never callback pixel references. The final lease survives SDK
shutdown and re-encoding its first part must give identical bytes. Earlier pins
are explicitly released; cleanup revokes all remaining pins.

Ordinary variable-length messages have no padding. Bulk framing is explicit and
may shrink/grow only at an agreed new image. There is no automatic link-quality
estimator. FILE permits missing-part requests, LIVE suppresses them. The file
preserves that mode; this executable opens no sockets. The independent test owns
the real loopback TCP/UDP transport.

```sh
cmake -S flashVM -B build-frame-parts -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-frame-parts --target flashvm_example_frame_parts flashvm_resource_frame_oracle
./build-frame-parts/frontend/flashvm_example_frame_parts frames.sparts
./build-frame-parts/frontend/flashvm_example_frame_parts --live live.sparts
ctest --test-dir build-frame-parts -R flashvm_resource_frame_parts_example --output-on-failure
```

Requires C89/C++98, CMake and a target SDL2 SDK. VM/bundled graphics/dependencies
link through the normal host target; Python is only a test dependency. No video,
window or audio device is initialized: the tutorial is muted/offscreen. Inside
the detached component use `cmake -S .` and the corresponding profile path.
`--help` opens neither file nor SDK. Output overwrites the specified file; failure
removes only partial output opened by this invocation. Expected stdout:
`exported_frames=3`.

## Format and ownership

This is a demonstration binary stream, not a general video container: legacy
tag-zero bindings first, then per image tag-20 metadata (the existing seventeen
fields plus source), compact OFFER_AT/ACCEPT_AT, and a known count of equal-size
tag-19 slots. Final raw part length is exact and slot padding zero. Metadata
preserves geometry; a lease ID in a file confers no native authority. Time values
sequence*40 are chosen tutorial metadata, not FPS measurement or A/V sync.

The private dispatcher registers only pure native cache handlers, so pumping it
inside this observer never executes arbitrary user VM work. Main owns file,
cache and exchanges; linked SWF bytes live through runner return and callback
pixels are borrowed only until capture. Output needs one 4096-byte caller slot
plus three 4096-byte cache slots; the exporter creates no second whole-image
buffer. Never hand a reused output pointer to a worker: queues must own their
bytes and enforce backpressure. The API sends nothing and never releases a pin
implicitly.

The test compares every RGBA byte with independent integer/zlib and SWF pixel
expectations, then feeds actual exported slots to the native receiver through
IPv4 UDP loopback with reordering, duplicates, damage and omission. FILE sends
a compact missing list on TCP and repeats only requested pieces; LIVE never
repairs. Loopback accepts the selected datagram sizes; arbitrary Internet paths
are not certified for 4096-byte UDP packets. No production listener,
authentication, congestion controller, video codec or remote window is installed.

| API | Ownership / выполнение |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L95) | Own the bounded private queue; no worker/VM startup. Release after adapter destruction. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_resource_frame_create` | Allocate three exact 4096-byte cache slots and register native handlers; partial failures free storage, dispatcher borrowed. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L108) | Explicitly grant READ to the trusted local session; wire metadata grants no authority. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_init`](../../../include/flashvm/scontrol.h#L74) | Copy real local buffer ceilings; no active profile or allocation. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_sbindings_payload_write`](../../../include/flashvm/sbindings.h#L43) | Encode two immutable direction-local type declarations; input is borrowed only during the call. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_srecord_write`](../../../include/flashvm/srecord.h#L66) | Wrap metadata/type declarations as compact checked records; output/used atomic on error. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_options_init` | Initialize the native playback descriptor before choosing mute/frame limit/interpreter. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_frame_observer_init` | Initialize exact observer ABI; callback borrows owner state through runner return. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_run_source_offscreen` | Run real linked SWF/raster synchronously without SDL video/events; all borrowed bytes live through return. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_resource_frame_capture` | Copy callback pixels into a spare cache slot; no source pointer retained, BUSY requires explicit host policy. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L115) | Copy one private cache request; accepted ID must later be taken or cancelled/taken. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_rdispatch_pump`](../../../include/flashvm/rdispatch.h#L119) | Execute only this private pure-cache handler on the owner, one bounded job; never pump arbitrary VM work here. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L126) | Copy and acknowledge the terminal reply, releasing the request slot. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_rdispatch_cancel`](../../../include/flashvm/rdispatch.h#L121) | Cancel queued work if local pumping fails; take still acknowledges the cancellation reply. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_resource_frame_read` | Strictly decode owned ACQUIRE metadata, including exact lease and geometry. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_propose_at`](../../../include/flashvm/scontrol.h#L101) | Request a future capture-sequence activation boundary; keep prior active profile unchanged. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_write`](../../../include/flashvm/scontrol.h#L67) | Encode a small tag-zero proposal/ACK, no slot padding. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_read`](../../../include/flashvm/scontrol.h#L69) | Verify service bytes and copy exactly one message; trailing records stay unconsumed. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_respond`](../../../include/flashvm/scontrol.h#L83) | Choose no-greater peer/local limits into pending ACK state. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_commit`](../../../include/flashvm/scontrol.h#L90) | Activate only exact/correlated ACK after reliable host queueing, with the same boundary. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_scontrol_active`](../../../include/flashvm/scontrol.h#L108) | Copy committed selected values; UNKNOWN before agreement; preserve outputs on errors. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_resource_frame_write_part` | Check current READ/lease owner and encode one agreed-size retained range without allocating or touching VM. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_sinteger_write`](../../../include/flashvm/srecord.h#L56) | Encode two portable fields for explicit lease release; no native-endian struct serialization. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_resource_frame_revoke` | Quiesced owner removes session rights and every remaining pin, including error paths. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_resource_frame_destroy` | Unregister and free cache only after callbacks; successful destroy nulls its pointer. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L131) | Free a quiesced queue after its adapters and terminal replies are gone. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |
| `fvm_desktop_shutdown` | Verify runner/SDL ownership ended; final native pixel export deliberately follows this call. [Contract](../../../doc/PLAYER_FRAME_PARTS.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:121](../../../include/flashvm/rdispatch.h#L121) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../../../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../../../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../../../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_pump` | function | [include/flashvm/rdispatch.h:119](../../../include/flashvm/rdispatch.h#L119) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:126](../../../include/flashvm/rdispatch.h#L126) |
| `fvm_sbindings_payload_write` | function | [include/flashvm/sbindings.h:43](../../../include/flashvm/sbindings.h#L43) |
| `fvm_scontrol_active` | function | [include/flashvm/scontrol.h:108](../../../include/flashvm/scontrol.h#L108) |
| `fvm_scontrol_commit` | function | [include/flashvm/scontrol.h:90](../../../include/flashvm/scontrol.h#L90) |
| `fvm_scontrol_init` | function | [include/flashvm/scontrol.h:74](../../../include/flashvm/scontrol.h#L74) |
| `fvm_scontrol_propose_at` | function | [include/flashvm/scontrol.h:101](../../../include/flashvm/scontrol.h#L101) |
| `fvm_scontrol_read` | function | [include/flashvm/scontrol.h:69](../../../include/flashvm/scontrol.h#L69) |
| `fvm_scontrol_respond` | function | [include/flashvm/scontrol.h:83](../../../include/flashvm/scontrol.h#L83) |
| `fvm_scontrol_write` | function | [include/flashvm/scontrol.h:67](../../../include/flashvm/scontrol.h#L67) |
| `fvm_sinteger_write` | function | [include/flashvm/srecord.h:56](../../../include/flashvm/srecord.h#L56) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../../../include/flashvm/srecord.h#L66) |
<!-- source-links:end -->
