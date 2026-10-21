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
| [`fvm_sslot_batch`](../../../include/flashvm/sslot.h#L38) | Inspect known fixed boundaries independently; callback views borrow input, no allocation/I/O; reject partial batch before callbacks. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_write`](../../../include/flashvm/smedia.h#L55) | Encode an exact bulk part into caller slot storage; metadata/extent INVALID, insufficient capacity LIMIT; output unchanged on failure. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_read`](../../../include/flashvm/smedia.h#L60) | Validate a full slot before exposing borrowed bytes for an immediate consumer; wrong bound tag UNKNOWN, malformed body INVALID, damaged checksum CHECKSUM. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_receiver_init`](../../../include/flashvm/smedia.h#L65) | Borrow an adequate caller object buffer and copy agreed identity; no buffer initialization or allocation; small capacity LIMIT preserves old state. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_accept`](../../../include/flashvm/smedia.h#L73) | Verify and copy a new matching part; exact duplicate idempotent, conflicting identity/bytes CONFLICT; error preserves buffer/state/complete. [Contract](../../../doc/MEDIA_SLOTS.md). |
| [`fvm_smedia_missing`](../../../include/flashvm/smedia.h#L80) | Enumerate bounded ascending FILE holes for host retry policy; LIVE returns zero holes. No network request or blocking; caller owns result array. [Contract](../../../doc/MEDIA_SLOTS.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_smedia_accept` | function | [include/flashvm/smedia.h:73](../../../include/flashvm/smedia.h#L73) |
| `fvm_smedia_missing` | function | [include/flashvm/smedia.h:80](../../../include/flashvm/smedia.h#L80) |
| `fvm_smedia_read` | function | [include/flashvm/smedia.h:60](../../../include/flashvm/smedia.h#L60) |
| `fvm_smedia_receiver_init` | function | [include/flashvm/smedia.h:65](../../../include/flashvm/smedia.h#L65) |
| `fvm_smedia_write` | function | [include/flashvm/smedia.h:55](../../../include/flashvm/smedia.h#L55) |
| `fvm_srecord_write` | function | [include/flashvm/srecord.h:66](../../../include/flashvm/srecord.h#L66) |
| `fvm_sslot_batch` | function | [include/flashvm/sslot.h:38](../../../include/flashvm/sslot.h#L38) |
<!-- source-links:end -->
