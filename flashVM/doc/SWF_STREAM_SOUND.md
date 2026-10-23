# Root SWF streaming PCM / Потоковый PCM корневого SWF — v0.558

## Scope / Область реализации

The C89 API in `include/flashvm/swf_stream_sound.h` inspects canonical FWS/CWS
bytes loaded by [fvm_load_memory](../include/flashvm/flashvm.h#L72). Each root SoundStreamHead/Head2 occurrence
starts a numbered segment; subsequent SoundStreamBlock tags belong to it until
another header. Only root tags are examined; nested DefineSprite bodies are
opaque. This is explicit inspection and decoding, not progressive network
loading or automatic MovieClip playback, seeking, synchronization or silence
insertion. NO_MEDIA requires no FFmpeg/device SDK.

The wire reference is Adobe's [SWF 19 specification, streaming sound chapter](https://open-flash.github.io/mirrors/swf-spec-19.pdf#page=183),
with the original [Adobe URL](https://www.adobe.com/content/dam/acom/en/devnet/pdf/swf-file-format-spec.pdf).
The implemented profile accepts original headers for ADPCM/MP3 metadata and
Head2 from SWF3; PCM format 3 and MP3 require SWF4. Compressed blocks remain
opaque and cannot be decoded by this API, even for a zero-frame request.

Each header/block has its one-based movie frame. Average sample count and
playback geometry are advisory. Actual PCM frame count comes from complete
payload bytes divided by source channel count and sample width. Blocks can have
unequal lengths or zero bytes; a missing frame creates no block. At most one
block is accepted per root movie frame, including across header changes.

All five operations scan the complete root tag stream before publishing results.
They validate outer record bounds, header fields, PCM alignment, header-before-
block ordering, one-block-per-frame, exact ShowFrame count and terminal End.
Unrelated tag bodies and compressed payload contents are not certified by this
validation. Earlier selected results are rejected if a later relevant control
is malformed. There is no parser cache or heap allocation.

## Ownership and execution / Владение и исполнение

Call on the owning VM thread. Null contexts and destruction/collection/release
phases return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); an unloaded context returns [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46).
The parser and decoder allocate nothing, invoke no host or script callbacks and
perform no GC or playback. Stream ordinals and views refer to the **current**
loaded movie; successful reload replaces their meaning. Failed load preserves
the previous canonical movie. No live decoder state is retained between calls.

[GC:] Header records contain only copied scalars. Block metadata is copied but
`data` borrows canonical native bytes until successful reload/context destruction;
ordinary GC alone does not invalidate them. Never retain this pointer across
reentrant host operations that can reload. PCM reads re-resolve their ordinals
and copy into caller storage before returning, with no intervening callback.
The written counter must not overlap PCM output or canonical source bytes.
Output shorts are owned by the caller and can survive VM destruction. Metadata
and destination storage must not alias canonical SWF bytes or each other.
Serialized graph transport does not transfer these native views/ordinals.

## FVM_SWFStreamSoundInfo

Set `struct_size` to its exact sizeof. `stream_index` is the zero-based root
header occurrence, `header_tag` is 18/45 and `frame` is one-based. `format`,
`sample_rate`, `bits_per_sample`, `channels` describe the source header;
`playback_sample_rate`, `playback_bits_per_sample`, `playback_channels` copy
advice without causing conversion. Rates use the existing nominal integer
5512/11025/22050/44100 convention, not a new fractional clock. `average_samples`
is the UI16 average, including zero; it never truncates/pads PCM. `latency_seek`
is copied as a signed MP3 value or zero for other formats. Opaque codec geometry
is header metadata, not proof of decoded compressed output geometry.

## FVM_SWFStreamSoundBlock

Set exact `struct_size`. `stream_index` and `block_index` identify a segment and
its zero-based encoded block; `frame` retains its movie position. `data` and
`data_size` cover the complete payload without codec-prefix removal. For PCM
formats 0/3, `sample_count_known` is 1 and `sample_count` is the actual number of
complete sample frames (stereo pairs). An explicit empty PCM block has known
zero frames. Compressed blocks have unknown count (flag 0 and count 0), including
MP3 blocks that require a separate codec-specific parser. The borrowed payload
lifetime follows the ownership section above.

## fvm_swf_stream_sound_count

**Arguments:** owning context and required `size_t *out`. **Success:** [FVM_OK](../include/flashvm/flashvm.h#L42)
copies the number of root header segments; zero is valid. **Errors:** common
context errors above, [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) for null output or malformed relevant
controls. The old count remains unchanged on failure. No resource handle is
created. This count does not enumerate nested sprites.

## fvm_swf_stream_sound_at

**Arguments:** context, zero-based segment ordinal, required correctly sized
[FVM_SWFStreamSoundInfo](../include/flashvm/swf_stream_sound.h#L10) output. **Success:** [FVM_OK](../include/flashvm/flashvm.h#L42) publishes the copied record.
**Errors:** [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47) for an absent ordinal after root validation; common
context/validation errors; [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) for null/wrong-size output.
All fields remain unchanged on error. No owned or borrowed byte pointer escapes.

## fvm_swf_stream_sound_block_count

**Arguments:** context, segment ordinal and required `size_t *out`.
**Success:** [FVM_OK](../include/flashvm/flashvm.h#L42) publishes encoded block occurrences in that segment, including
empty blocks. A header with no blocks returns zero; missing frames are excluded.
**Errors:** [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47) for an absent segment; [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) for null
output/malformed controls and common context errors. Failure preserves the count.

## fvm_swf_stream_sound_block_at

**Arguments:** context, segment ordinal, segment-relative block ordinal and
required correctly sized [FVM_SWFStreamSoundBlock](../include/flashvm/swf_stream_sound.h#L27) output. **Success:** [FVM_OK](../include/flashvm/flashvm.h#L42)
copies metadata plus the borrowed payload span. **Errors:** [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47) if the
segment or block is absent; [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) for null/wrong-size output or
malformed controls, plus common context errors. Failure preserves every field.
Do not use a previously borrowed payload after successful reload.

## fvm_swf_stream_sound_read_pcm

**Arguments:** context, segment/block ordinals, zero-based first frame within
that block, requested frame count, caller `short *destination`, capacity in
**channel values**, required `size_t *written_values`.

`written_values` is cleared before context/record validation. [FVM_OK](../include/flashvm/flashvm.h#L42) copies
exactly the requested range and reports frames × channels. Zero-frame requests
allow null destination, but still resolve/validate a supported PCM block and
require first ≤ actual sample_count. End-of-block is not an implicit short read;
requesting beyond it returns [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Null written output is invalid.
Capacity and multiplication bounds are checked before any PCM write. On every
error PCM remains unchanged and written is zero.

Formats 0/3, U8 and S16, mono/stereo are supported. U8 expands (byte−128)×256;
format 0 follows host byte order and format 3 is little-endian. Returned native
shorts require octet-based signed-16 representation; incompatible hosts return
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Missing segment/block returns [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47). Compressed codecs
return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). All other context/header/range/capacity errors follow
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) as described above. There is no state advancement, source
resampling, advisory playback conversion, gain, timestamp or output registration.

The implementation shares [fvm_swf_sound_decode_pcm](../src/swf/sound_pcm.hpp#L9) with DefineSound, including
its complete-range preflight. DefineSound ADPCM support is unchanged; it does
not imply ADPCM streaming support. Use bounded reads followed by
[fvm_audio_channel_write](../include/flashvm/multimedia.h#L84) to your explicitly owned output; handle real-time clock,
gaps, segment geometry changes, seeking and underruns in the host.

## Русский контракт

API читает корневые SoundStreamHead/Head2 и блоки SoundStreamBlock из текущих
canonical FWS/CWS. Каждый заголовок начинает новый segment с индексом от нуля;
блоки внутри него также нумеруются от нуля. Поле frame — настоящий номер кадра
от единицы. Заголовок содержит среднее количество samples и рекомендации
playback; они не определяют длину PCM блока и не вызывают конверсию. Пропущенные
кадры не превращаются в тишину; явный пустой блок сохраняется. Несколько блоков
в одном movie frame запрещены, включая смену заголовка внутри кадра.

[FVM_SWFStreamSoundInfo](../include/flashvm/swf_stream_sound.h#L10) копирует source/playback geometry, среднее и signed MP3
latency. [FVM_SWFStreamSoundBlock](../include/flashvm/swf_stream_sound.h#L27) копирует frame/индексы и borrowed data/data_size.
Для PCM sample_count_known=1, sample_count — фактические sample frames по длине
payload; для compressed это 0/0 и bytes остаются opaque. Nominal rates совпадают
с DefineSound (5512/11025/22050/44100), дробные часы не вводятся.

Все выходные структуры требуют точного struct_size. Функции count/at/block_count/
block_at сохраняют выходы при любой ошибке; отсутствующий segment/block даёт
[FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47). Count без заголовков успешно возвращает ноль. Заголовок без блоков
имеет block_count=0. Проверка всего корневого потока выполняется до публикации,
включая поздние malformed controls, ShowFrame count, End и PCM alignment; она
не проверяет содержимое посторонних тегов или compressed payload.

read_pcm принимает segment, block, first, frames и caller short buffer; capacity
и written измеряются channel values, first/frames — sample frames. Written
обязателен и обнуляется сразу. Успех выдаёт весь запрошенный диапазон, а не
короткий EOF-read. Zero frames допускает null buffer, но всё равно проверяет
поддерживаемый блок и first. За границей, при малом capacity/null destination
для ненулевого запроса возвращается [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) без изменений PCM.
Поддержаны native/LE U8/S16 mono/stereo; compressed stream, включая ADPCM/MP3,
возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) даже для zero-read. Пропавшие ordinals дают [FVM_NOT_FOUND](../include/flashvm/flashvm.h#L47).

[GC:] Ни parser, ни read не выделяют память, не вызывают callbacks/скрипт/GC и
не сохраняют скрытое состояние. Вызовы только на VM thread вне destruction,
collection/release; нарушение — [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45), незагруженный context —
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Borrowed payload живёт до успешного reload/context_destroy;
обычная GC не меняет canonical bytes. Счётчик written также не должен перекрывать PCM buffer или canonical source.
Не используйте views после reentrant
reload и не перекрывайте output с source. Caller PCM независим от VM после
копирования. Ordinals после reload относятся к новому файлу, а не к старому.

Head2 разрешён с SWF3, LE PCM/MP3 — с SWF4; оригинальный Head принимает только
ADPCM/MP3 metadata с 16-bit flags. Advisory playback не меняет source geometry.
Планировщик, MovieClip/sprite integration, progressive loading, resampling,
seek/gap sync и compressed decoder здесь не реализованы. Для host pipeline
используйте bounded read → [fvm_audio_channel_write](../include/flashvm/multimedia.h#L84) и явную остановку output.

## Example and evidence / Пример и проверки

[swf_stream](../examples/multimedia/swf_stream/README.md) exports literal variable
stereo PCM blocks through real stdio, with a frame gap and explicit empty block.
It uses caller-sized source slices and actual registered stopping. See its README
for setup, expected bytes, API links and ownership/error cleanup.

`swf_stream_sound` covers SWF versions, source widths/layouts, header changes,
unequal/empty blocks, gaps, opaque codecs, signed latency, malformed later tags,
range/capacity transactionality and missing ordinals. `swf_stream_sound_api`
compiles strict C89 and compares FWS6/CWS6 results. `swf_stream_sound_allocation`
denies allocation for all five APIs and exercises GC/reload/lifecycle limits.
`swf_stream_sound_bounds` checks extended-length payloads, every truncated
canonical span, oversized lengths/ranges and failed-load preservation. The
canonical truncation fixture deliberately injects internal storage faults that
the outer loader normally rejects first. These are parser/PCM tests, not full
audio playback or foreign-platform acceptance.

## Prepared immutable PCM index / Подготовленный неизменяемый индекс PCM

Since v0.576 capture also preserves compressed segments without decoding them;
see [MP3 prefix and encoded snapshot contracts](SWF_STREAM_MP3.md).

The optional [`FVM_SWFStreamSoundSnapshot`](../src/swf/sound_stream.cpp#L18) removes repeated root scans for incremental
reads. Open performs two linear scans of the loaded root: full validation and size
measurement, then copying into an exact block index and raw byte store. It does
not expand PCM to S16 or copy unrelated segments. Preparation remains on the
context's owning thread, outside terminal/collecting GC phases, with no concurrent
load or mutation. It invokes no host callback or collection.

Необязательный снимок устраняет повторное сканирование SWF при чтении короткими
порциями. Подготовка выполняет два линейных прохода: полную проверку и подсчёт,
затем копирование индекса и исходных закодированных байтов. Чужие сегменты не копируются,
предварительного расширения в S16 нет. Подготовка выполняется в потоке владельца
контекста; параллельные загрузка/изменение и терминальные фазы GC недопустимы.

The snapshot functions are declared in `include/flashvm/swf_stream_sound.h`:

- `fvm_swf_stream_sound_snapshot_open(context, stream, out)` requires a valid loaded
  context, zero-based segment and nonnull handle-output pointer. Clears `*out` on
  every failure; returns OK with a new independently owned handle, NOT_FOUND for
  a missing segment, UNSUPPORTED for an unloaded context,
  INVALID_ARGUMENT for malformed controls or invalid context/output, OUT_OF_MEMORY
  for allocation failure or budget overflow. All later root controls validate
  before allocation. The budget is configured `memory_limit_bytes`, or the existing
  default SWF limit when zero. It bounds raw payload bytes plus
  `block_count * sizeof(FVM_SWFStreamSoundEntry)`; wrapper/allocator overhead is
  excluded. This is a per-snapshot cap, not aggregate accounting across handles.
- `fvm_swf_stream_sound_snapshot_info(snapshot, out)` copies the original header.
  `out->struct_size` must equal `sizeof(FVM_SWFStreamSoundInfo)`. Returns OK or
  INVALID_ARGUMENT for null/bad size, leaving metadata unchanged on error.
- `fvm_swf_stream_sound_snapshot_block_count(snapshot, out)` copies the segment's
  number of explicit blocks. Returns OK or INVALID_ARGUMENT for null arguments;
  output is unchanged on error. Header-only segments return zero blocks.
- `fvm_swf_stream_sound_snapshot_block_at(snapshot, ordinal, out)` requires the
  exact [`FVM_SWFStreamSoundBlock`](../include/flashvm/swf_stream_sound.h#L27) struct size. Returns OK, NOT_FOUND for an absent
  ordinal, INVALID_ARGUMENT for null/bad size. Output is unchanged on error.
  `data` points to original encoded bytes, retaining PCM endian/width when applicable, until close;
  it is never a decoded S16 buffer. Empty blocks have a nonnull zero-length view.
- `[fvm_swf_stream_sound_snapshot_read_pcm](../include/flashvm/swf_stream_sound.h#L148)(snapshot, ordinal, first, frames, out,
  capacity, written)` decodes to caller-owned native interleaved S16. Block index
  is constant-time; conversion visits requested samples only. Offsets/counts are
  sample frames, capacity/written are channel values. Required `written` is
  cleared before validation. Returns OK, NOT_FOUND for absent ordinal,
  INVALID_ARGUMENT for null handle/written, range/capacity/required destination,
  or UNSUPPORTED for a compressed codec or unsupported native representation. Destination is unchanged
  on failure. Zero frames at the block end and a null destination are valid.
- `fvm_swf_stream_sound_snapshot_close(snapshot)` releases the owned native index
  and bytes; returns void and accepts null. All borrowed views become invalid.
  Double close or any subsequent use of a released handle is invalid.

Контракты функций: open обнуляет выходной указатель, проверяет весь root и лимит,
возвращает независимый handle либо NOT_FOUND / UNSUPPORTED / INVALID_ARGUMENT /
OUT_OF_MEMORY. info и block_count копируют заголовок/число блоков; block_at
возвращает исходные байты, принадлежащие снимку до close. Для info/block_at нужен
точный struct_size; при ошибке выходная метаинформация не меняется. read_pcm
принимает смещения в sample frames, ёмкость и written в отдельных значениях
каналов, сначала обнуляет written и не меняет PCM при ошибке. Нулевой диапазон
в конце допустим. close освобождает снимок, null допустим. Лимит учитывает байты
payload и индекс блоков, исключая оболочку/накладные расходы аллокатора; каждый
снимок ограничивается отдельно. Пустой сегмент имеет ноль блоков.

After open, no operation needs the originating context or runtime: ordinary GC,
successful reload and destruction of both owners leave the snapshot valid.
Queries, reads and close allocate nothing and invoke no VM/host callbacks.
Immutable reads may run concurrently with separate outputs when the host protects
handle lifetime; close must never race any operation. Output buffers/counts must
not alias snapshot storage or each other. No clock, resampling, automatic silence,
compressed decoder or device acceptance is added.

После подготовки GC, новая загрузка и уничтожение исходных context/runtime не
влияют на снимок. Запросы, чтение и close не выделяют память и не вызывают VM,
GC или переходники. Параллельное чтение допустимо с отдельными буферами и защитой
времени жизни со стороны host; close не должен пересекаться с операциями.
Выходы не должны перекрывать друг друга или память снимка. Планировщик, ресемплинг,
тишина для пропусков и декодирование сжатых потоков не добавляются.

The [swf_stream tutorial](../examples/multimedia/swf_stream/README.md) demonstrates
`--prepared`: it destroys the source VM and exports through a new unloaded VM.
## MP3 prefix inspection / MP3-префиксы

[`fvm_swf_stream_sound_mp3_block_at`](../include/flashvm/swf_stream_sound.h#L51) and
[`fvm_swf_stream_sound_snapshot_mp3_block_at`](../include/flashvm/swf_stream_sound.h#L146) inspect declared count, signed seek
and the MPEG span after the prefix; they do not decode it. Полные контракты,
владение, статусы и ограничения: [SWF_STREAM_MP3.md](SWF_STREAM_MP3.md).
[Sequential native PCM reader / Последовательный native PCM reader](STREAM_PCM_READER.md).
## Explicit decoded preparation / Явная подготовка декодированного снимка

Since v0.763, [`fvm_swf_stream_sound_snapshot_open_pcm`](../include/flashvm/swf_stream_sound.h#L87) additionally normalizes whole ADPCM packets to native S16. The original-wire contracts above apply to [`fvm_swf_stream_sound_snapshot_open`](../include/flashvm/swf_stream_sound.h#L65); the new explicit API returns decoded payloads and normalized format/counts instead. Detailed ownership, budget, unsupported partial packets and oracle checks: [EN](STREAM_ADPCM.md) / [RU](STREAM_ADPCM.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:84](../include/flashvm/multimedia.h#L84) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `FVM_NOT_FOUND` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_swf_sound_decode_pcm` | function | [src/swf/sound_pcm.hpp:9](../src/swf/sound_pcm.hpp#L9) |
| `fvm_swf_stream_sound_at` | function | [include/flashvm/swf_stream_sound.h:55](../include/flashvm/swf_stream_sound.h#L55) |
| `fvm_swf_stream_sound_block_at` | function | [include/flashvm/swf_stream_sound.h:59](../include/flashvm/swf_stream_sound.h#L59) |
| `fvm_swf_stream_sound_block_count` | function | [include/flashvm/swf_stream_sound.h:57](../include/flashvm/swf_stream_sound.h#L57) |
| `fvm_swf_stream_sound_count` | function | [include/flashvm/swf_stream_sound.h:53](../include/flashvm/swf_stream_sound.h#L53) |
| `fvm_swf_stream_sound_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:51](../include/flashvm/swf_stream_sound.h#L51) |
| `fvm_swf_stream_sound_read_pcm` | function | [include/flashvm/swf_stream_sound.h:61](../include/flashvm/swf_stream_sound.h#L61) |
| `fvm_swf_stream_sound_snapshot_block_at` | function | [include/flashvm/swf_stream_sound.h:144](../include/flashvm/swf_stream_sound.h#L144) |
| `fvm_swf_stream_sound_snapshot_block_count` | function | [include/flashvm/swf_stream_sound.h:142](../include/flashvm/swf_stream_sound.h#L142) |
| `fvm_swf_stream_sound_snapshot_close` | function | [include/flashvm/swf_stream_sound.h:173](../include/flashvm/swf_stream_sound.h#L173) |
| `fvm_swf_stream_sound_snapshot_info` | function | [include/flashvm/swf_stream_sound.h:140](../include/flashvm/swf_stream_sound.h#L140) |
| `fvm_swf_stream_sound_snapshot_mp3_block_at` | function | [include/flashvm/swf_stream_sound.h:146](../include/flashvm/swf_stream_sound.h#L146) |
| `fvm_swf_stream_sound_snapshot_open` | function | [include/flashvm/swf_stream_sound.h:65](../include/flashvm/swf_stream_sound.h#L65) |
| `fvm_swf_stream_sound_snapshot_open_pcm` | function | [include/flashvm/swf_stream_sound.h:87](../include/flashvm/swf_stream_sound.h#L87) |
| `fvm_swf_stream_sound_snapshot_read_pcm` | function | [include/flashvm/swf_stream_sound.h:148](../include/flashvm/swf_stream_sound.h#L148) |
| `FVM_SWFStreamSoundBlock` | struct | [include/flashvm/swf_stream_sound.h:27](../include/flashvm/swf_stream_sound.h#L27) |
| `FVM_SWFStreamSoundEntry` | struct | [src/swf/sound_stream_internal.hpp:7](../src/swf/sound_stream_internal.hpp#L7) |
| `FVM_SWFStreamSoundInfo` | struct | [include/flashvm/swf_stream_sound.h:10](../include/flashvm/swf_stream_sound.h#L10) |
| `FVM_SWFStreamSoundSnapshot` | struct | [src/swf/sound_stream_internal.hpp:15](../src/swf/sound_stream_internal.hpp#L15) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
