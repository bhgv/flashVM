# Bulk object parts and compact control

Runnable C89 example without VM, SDL, codecs or sockets. [Contract](../../../doc/MEDIA_SLOTS.md).
[Русский](README.ru.md). [main.c](main.c), [limits.h](limits.h), [catalog](../../README.md#example-embedding-media_parts).

Three file parts share one native batch. The entire middle slot is destroyed,
including its length, while the final slot remains readable. The receiver lists
only the missing middle part, retries it, and compares every completed byte.
LIVE policy permits omission at a host deadline and never lists retry requests.
A separate small control message uses an ordinary variable-length record with
no 1024-byte padding. Fixed slots are an opt-in bulk profile only.

main owns every array and the receiver. The callback is synchronous, allocating
nothing, blocking on nothing and transferring no pointer to another thread.
Piece.data borrows its slot until input changes; accept copies into caller
storage. Holes are uninitialized and must not be treated as ready bytes. There
are no GC roots, SDK startup/shutdown or library allocations to free.

A real application supplies authenticated identity/tag/profile agreement,
TCP/UDP transport, pacing, retry limits and clocks/deadlines. This tutorial does
not decode or play audio/video.

```sh
cmake -S flashVM -B build-media -DFVM_BUILD_EXAMPLES=ON
cmake --build build-media --target flashvm_examples_embedding_media_parts_main
./build-media/flashvm_examples_embedding_media_parts_main
```

Inside the detached component use `cmake -S .`. Requires C89 and CMake; links only flashvm_srecord.

Expected / ожидается: `PASS compact control, bulk batch recovery, file retry and live no-retry policy`.

| API | Ownership / выполнение |
| --- | --- |
| [`fvm_srecord_write`](../../../include/flashvm/srecord.h#L66) | Encode small control messages with their actual variable length; caller owns output, atomic LIMIT/INVALID errors, no blocking. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_sslot_batch`](../../../include/flashvm/sslot.h#L40) | Inspect known fixed boundaries independently; callback views borrow input, no allocation/I/O; reject partial batch before callbacks. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_write`](../../../include/flashvm/smedia.h#L64) | Encode an exact bulk part into caller slot storage; metadata/extent INVALID, insufficient capacity LIMIT; output unchanged on failure. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_read`](../../../include/flashvm/smedia.h#L69) | Validate a full slot before exposing borrowed bytes for an immediate consumer; wrong bound tag UNKNOWN, malformed body INVALID, damaged checksum CHECKSUM. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_receiver_init`](../../../include/flashvm/smedia.h#L74) | Borrow an adequate caller object buffer and copy agreed identity; no buffer initialization or allocation; small capacity LIMIT preserves old state. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_accept`](../../../include/flashvm/smedia.h#L82) | Verify and copy a new matching part; exact duplicate idempotent, conflicting identity/bytes CONFLICT; error preserves buffer/state/complete. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_missing`](../../../include/flashvm/smedia.h#L89) | Enumerate bounded ascending FILE holes for host retry policy; LIVE returns zero holes. No network request or blocking; caller owns result array. [Contract](../../../doc/MEDIA_SLOTS.md). |
Bulk geometry is first agreed through compact tag-zero service records. Both
ends have separate exchange state; a proposal does not activate data framing.
The tutorial requests 1024/768 under a local 4096 ceiling. The same profile API
supports larger parts; the native contract checks 4096/4000 and a 64-KiB slot.

| API | Ownership / выполнение |
| --- | --- |
| [`fvm_scontrol_init`](../../../include/flashvm/scontrol.h#L74) | Copy local capacity maxima without activating payload framing. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_scontrol_propose`](../../../include/flashvm/scontrol.h#L78) | Start one increasing correlation ID and return an inactive offer; overlapping request CONFLICT. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_scontrol_write`](../../../include/flashvm/scontrol.h#L67) | Encode a compact checked tag-zero offer/ACK into caller storage, atomic on error. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_scontrol_read`](../../../include/flashvm/scontrol.h#L69) | Verify one service record and copy metadata; unknown services UNKNOWN, no implicit state changes. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_scontrol_respond`](../../../include/flashvm/scontrol.h#L83) | Select bounded peer/local quotas into pending state; no allocation or transport call. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_scontrol_commit`](../../../include/flashvm/scontrol.h#L90) | Activate only the exact/correlated bounded ACK after reliable host queueing; invalid ACK preserves old state. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_write_profile`](../../../include/flashvm/smedia.h#L94) | Encode an exact part using agreed caller profile and output capacity; no slot-sized scratch allocation. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_receiver_init_profile`](../../../include/flashvm/smedia.h#L100) | Copy agreed geometry/identity while borrowing object storage; INVALID/LIMIT preserve receiver. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_read_profile`](../../../include/flashvm/smedia.h#L98) | Verify the configured slot and part extent; output borrows input only on OK. [Contract](../../../doc/MEDIA_SLOTS.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_scontrol_commit` | function | [include/flashvm/scontrol.h:90](../../../include/flashvm/scontrol.h#L90) |
| `fvm_scontrol_init` | function | [include/flashvm/scontrol.h:74](../../../include/flashvm/scontrol.h#L74) |
| `fvm_scontrol_propose` | function | [include/flashvm/scontrol.h:78](../../../include/flashvm/scontrol.h#L78) |
| `fvm_scontrol_read` | function | [include/flashvm/scontrol.h:69](../../../include/flashvm/scontrol.h#L69) |
| `fvm_scontrol_respond` | function | [include/flashvm/scontrol.h:83](../../../include/flashvm/scontrol.h#L83) |
| `fvm_scontrol_write` | function | [include/flashvm/scontrol.h:67](../../../include/flashvm/scontrol.h#L67) |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:82](../../../include/flashvm/smedia.h#L82) |
| `fvm_smedia_missing` | function | [include/flashvm/smedia.h:89](../../../include/flashvm/smedia.h#L89) |
| `fvm_smedia_read` | function | [include/flashvm/smedia.h:69](../../../include/flashvm/smedia.h#L69) |
| `fvm_smedia_read_profile` | function | [include/flashvm/smedia.h:98](../../../include/flashvm/smedia.h#L98) |
| `fvm_smedia_receiver_init` | function | [include/flashvm/smedia.h:74](../../../include/flashvm/smedia.h#L74) |
| `fvm_smedia_receiver_init_profile` | function | [include/flashvm/smedia.h:100](../../../include/flashvm/smedia.h#L100) |
| `fvm_smedia_write` | function | [include/flashvm/smedia.h:64](../../../include/flashvm/smedia.h#L64) |
| `fvm_smedia_write_profile` | function | [include/flashvm/smedia.h:94](../../../include/flashvm/smedia.h#L94) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../../../include/flashvm/srecord.h#L66) |
| `fvm_sslot_batch` | function | [include/flashvm/sslot.h:40](../../../include/flashvm/sslot.h#L40) |
<!-- source-links:end -->
