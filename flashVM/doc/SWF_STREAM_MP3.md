# Encoded streaming snapshots and MP3 prefixes / Сжатые снимки и MP3-заголовки — v0.576

## English

[`FVM_SWFStreamSoundSnapshot`](../src/swf/sound_stream.cpp#L18) now copies/indexes any encoded root stream segment
accepted by the existing header/tag parser, including ADPCM, MP3 and opaque codec
identities. It preserves exact original payload bytes; it does not decode, certify
or play compressed audio. Existing PCM extraction is unchanged. Both live and
snapshot `read_pcm` reject compressed blocks with UNSUPPORTED, even for zero
frames or an out-of-range first-frame value. Missing block ordinals still return
NOT_FOUND. [General stream contracts](SWF_STREAM_SOUND.md).

### FVM_SWFStreamSoundMP3Block

Declared in [swf_stream_sound.h](../include/flashvm/swf_stream_sound.h).
Initialize `struct_size` to exactly sizeof this type. `stream_index`, `block_index`
and `frame` identify the encoded segment, block ordinal and one-based movie frame.
`declared_samples` copies the UI16 prefix count: audio sample frames (stereo pairs),
not channel values and not a verified decoded length. `seek_samples` copies the
following SI16 without unsigned wrap. It is a per-block adjustment; the header's
`latency_seek` is copied separately and is never silently added here.
`data`/`data_size` borrow the MPEG span **after** the four-byte SWF prefix. Empty
spans are valid structural output; no MPEG frame sync, duration, sample count,
rate, channel layout, reservoir dependencies or decodability is verified.
An invalid MPEG span can therefore still produce successful prefix inspection.

### fvm_swf_stream_sound_mp3_block_at

**Arguments:** owning context, zero-based segment ordinal, segment-relative block
ordinal and a nonnull, exactly sized [`FVM_SWFStreamSoundMP3Block`](../include/flashvm/swf_stream_sound.h#L39) output.
**Success:** OK publishes the copied prefix fields and borrowed MPEG span after
validating the entire root timeline's existing structural/PCM controls.
**Errors:** INVALID_ARGUMENT for null/wrong-size output, invalid/terminal context,
malformed root controls or a selected MP3 payload shorter than four bytes;
UNSUPPORTED for an unloaded context or a selected non-MP3 block; NOT_FOUND for
an absent segment/block. The entire output is unchanged on every failure.
No output handle is created, allocation occurs, host/script callback runs or GC
is triggered. The byte span lasts until successful reload/context destruction.
Use only on the owning VM thread, without concurrent reload or mutation.

### fvm_swf_stream_sound_snapshot_mp3_block_at

**Arguments:** an open [`FVM_SWFStreamSoundSnapshot`](../src/swf/sound_stream.cpp#L18), block ordinal and correctly
sized nonnull output. **Success:** OK publishes the indexed prefix/MPEG view.
**Errors:** INVALID_ARGUMENT for null snapshot/output, wrong size or a selected
MP3 prefix shorter than four bytes; NOT_FOUND for an absent block; UNSUPPORTED
for a selected non-MP3 codec. Output is unchanged on failure. There is no
allocation, root rescan, VM access, callback or GC. The originating VM may already
be destroyed. The span lasts until [`fvm_swf_stream_sound_snapshot_close`](../include/flashvm/swf_stream_sound.h#L173).
Concurrent immutable reads are possible while an external owner guarantees that
close cannot race with them; the API supplies no locking or reference counting.

### Capture and ownership

[`fvm_swf_stream_sound_snapshot_open`](../include/flashvm/swf_stream_sound.h#L65) retains its two-pass preparation and budget:
encoded payload bytes plus `block_count * sizeof(FVM_SWFStreamSoundEntry)` are
bounded by runtime `memory_limit_bytes` or the default SWF limit. Small wrapper
and allocator overhead remain outside that accounting. It clears `*out` on
failure and publishes no partial snapshot. A missing segment returns NOT_FOUND;
compressed format alone no longer returns UNSUPPORTED. Malformed controls and
invalid contexts return INVALID_ARGUMENT; allocation/budget failure returns
OUT_OF_MEMORY; an unloaded context returns UNSUPPORTED. Close is required and
accepts null. Copied snapshots have no managed or source-VM references.

The ordinary raw block APIs/capture leave compressed payloads opaque. They do
not start rejecting short MP3 prefixes globally. Only a requested MP3-prefix
lookup checks its selected four-byte prefix; unrelated malformed MPEG bytes or
short prefixes in other blocks are not certified by a successful lookup.
Do not overlap output metadata with source bytes or another output record. Do
not retain live views across reload or snapshot views across close. There are
no new codec, FFmpeg, device or OS dependencies.

## Русский

Снимок [`FVM_SWFStreamSoundSnapshot`](../src/swf/sound_stream.cpp#L18) теперь сохраняет любой закодированный корневой
сегмент, допустимый существующим парсером заголовков/тегов: ADPCM, MP3 и opaque
идентификаторы кодеков. Сохраняются точные исходные байты. Декодирования,
проверки MPEG и воспроизведения нет. PCM работает по прежним правилам.
`read_pcm` для сжатого блока возвращает UNSUPPORTED даже при нулевом запросе
или большом first; отсутствующий ordinal по-прежнему даёт NOT_FOUND.

[`FVM_SWFStreamSoundMP3Block`](../include/flashvm/swf_stream_sound.h#L39) требует точный `struct_size`. Индексы и `frame`
обозначают сегмент, блок и movie frame от единицы. `declared_samples` — UI16 из
потока в sample frames/стереопарах, не доказанное число декодированных samples.
`seek_samples` — следующий SI16 со знаком. Header `latency_seek` остаётся отдельным
полем; автоматического суммирования нет. `data/data_size` — borrowed MPEG-байты
после четырёхбайтового SWF-префикса. Пустое тело разрешено. Синхрометки MPEG,
длительность, bit reservoir, частота, число каналов и декодируемость не проверяются.

`fvm_swf_stream_sound_mp3_block_at(context, stream, block, out)` сначала проверяет
весь корневой поток по существующим правилам, затем выбранный MP3-префикс.
Нужны owning VM thread, загруженный context и выход с точным размером. OK выдаёт
метаданные и borrowed view до успешного reload/context_destroy. Нет аллокаций,
callbacks, скрипта и GC. INVALID_ARGUMENT: null/неверный размер, терминальный
context, повреждённые controls или выбранный MP3 payload короче четырёх байтов.
UNSUPPORTED: незагруженный context или выбранный другой кодек. NOT_FOUND:
нет сегмента/блока. При любой ошибке весь выход остаётся прежним.

`fvm_swf_stream_sound_snapshot_mp3_block_at(snapshot, block, out)` делает тот же
разбор по независимому индексу без VM, сканирования тегов и аллокаций. Работает
после уничтожения VM. INVALID_ARGUMENT: null snapshot/out, неверный размер или
короткий выбранный префикс; NOT_FOUND: нет блока; UNSUPPORTED: другой кодек.
Выход неизменен при ошибке. View живёт до snapshot_close; параллельное чтение
допустимо только при внешней гарантии отсутствия одновременного close.
Внутренних блокировок/счётчиков ссылок нет.

snapshot_open по-прежнему выполняет два прохода, обнуляет *out при ошибке и
проверяет бюджет bytes + index; wrapper/allocator overhead в него не входят.
Сжатый формат сам по себе больше не даёт UNSUPPORTED. NOT_FOUND означает
отсутствующий сегмент; INVALID_ARGUMENT — неверные аргументы/controls/context;
OUT_OF_MEMORY — бюджет/аллокация; UNSUPPORTED — незагруженный context.
Подготовка идёт на owning thread, без callbacks/GC. Снимок явно освобождается,
не держит managed-ссылок и адреса исходной VM. null при close допустим.

Обычный raw block/capture не проверяет содержимое compressed bytes: четыре байта
проверяет только специальный запрос выбранного MP3-префикса. Выходы не должны
перекрывать источник. Нельзя использовать live view после reload, а snapshot
view после close. Новых зависимостей от FFmpeg, устройства или ОС нет.

## Example and verification / Пример и проверки

[swf_stream_mp3](../examples/multimedia/swf_stream_mp3/README.md) — annotated C89
metadata-only tutorial with explicit `--demo`; its synthetic MPEG bytes are not
a playable audio fixture. `tests/swf_stream_mp3.cpp` covers SWF4–8, both header
tags, mono/stereo advice, six signed seek boundaries, five declared counts,
short prefixes, empty MPEG spans, opaque codecs and reload/teardown lifetime.
`tests/swf_stream_mp3_allocation.cpp` covers capture OOM/retry and inspection
under denied allocations after VM destruction. The existing strict C89 and PCM
snapshot regression tests are extended. These checks do not certify MP3 playback,
original-player equivalence, automatic MovieClip clocks or foreign platforms.

Current Linux v0.576 run: full C89/C++98 build, 1204 passed and four explicit
skips (1208 registered). The actual annotated `--demo` runs after destroying its
source VM. No sanitizer, native foreign-OS, genuine MP3 decoding or audible output
is claimed. Current checkpoint evidence also appears in [ROADMAP_STATUS.md](ROADMAP_STATUS.md).

Specification reference: Adobe SWF 19, MP3STREAMSOUNDDATA and MP3SOUNDDATA
([archived format reference](https://open-flash.github.io/mirrors/swf-spec-19.pdf)).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_swf_stream_sound_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:51](../include/flashvm/swf_stream_sound.h#L51) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:173](../include/flashvm/swf_stream_sound.h#L173) |
| `fvm_swf_stream_sound_snapshot_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:146](../include/flashvm/swf_stream_sound.h#L146) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../include/flashvm/swf_stream_sound.h#L65) |
| `FVM_SWFStreamSoundEntry` | struct | [src/swf/sound_stream_internal.hpp:7](../src/swf/sound_stream_internal.hpp#L7) |
| `FVM_SWFStreamSoundMP3Block` | struct | [include/flashvm/swf_stream_sound.h:39](../include/flashvm/swf_stream_sound.h#L39) |
| `FVM_SWFStreamSoundSnapshot` | struct | [src/swf/sound_stream_internal.hpp:15](../src/swf/sound_stream_internal.hpp#L15) |
<!-- source-links:end -->
