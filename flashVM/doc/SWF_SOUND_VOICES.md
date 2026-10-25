# Prepared incremental sound voices / Порционное чтение подготовленного голоса

## Scope and ownership

[`FVM_SWFSoundVoice`](../src/swf/sound_voice.cpp#L12) is an explicitly host-owned native snapshot of one root
StartSound's decoded source interval and ordered envelope. It renders the same
PCM/ADPCM ranges, repetitions, gains and fixed clocks as
[fvm_swf_sound_event_submit](SWF_SOUND_PLAYBACK.md), but retains the source only
once and produces caller-sized S16 chunks. Large play counts do not expand stored
PCM. This is not decoding SoundStreamBlock tags, automatic MovieClip playback,
audio scheduling or mixing. It neither registers an output nor participates in
context-wide stop/no-multiple. Hosts retain responsibility for choosing voices,
clocking and synchronous output through their bridge.

[GC:] A successful snapshot contains no context/runtime pointer, managed value,
root, borrowed SWF bytes or output userdata. It remains valid after GC, successful
or failed reload, context/runtime destruction. Closing the originating VM does
not close a snapshot: the host must call [fvm_swf_sound_voice_close](../include/flashvm/swf_sound.h#L162) exactly once.
A null close succeeds; double close, stale pointers and concurrent access are
invalid usage. Closing enters no VM, collector or host output callback.

Open runs on the owning VM thread outside destruction/GC collection/release.
It performs native allocation but no managed allocation, collection or output
callback; ordinary nonterminal native callbacks may prepare a snapshot. Once
open returns, a handle may be transferred to another host thread with proper
publication/synchronization. All operations on the same handle must be serialized
with each other, including info and close. Separate snapshots are independent.
Caller destinations must not alias the handle, internal native state, canonical
SWF bytes or the written-count output.

## FVM_SWFSoundVoiceInfo

Set `struct_size` to sizeof([FVM_SWFSoundVoiceInfo](../include/flashvm/swf_sound.h#L83)). The getter copies every field:

| Field | Meaning |
| --- | --- |
| sound_id | Captured root resource ID, without a live VM reference. |
| flags | Original SOUNDINFO flags; no-multiple is preserved but not executed. |
| sample_rate, channels | Captured nominal rate and mono/stereo interleaving. |
| source_first | Converted first source frame, using the 44100 Hz control clock. |
| frames_per_play | Original selected interval length, even for explicit zero plays. |
| play_count | Total requested plays, including zero. |
| play_index | Current zero-based play, or play_count after nonempty exhaustion. |
| frame_in_play | Next source-relative frame; zero after completing the final play. |
| finished | Zero plays, empty source interval or all repetitions consumed. |

For an empty interval, finished is true from open even when play_index is zero
and play_count is positive. An empty/zero-play snapshot still describes original
source geometry. Source pointers are deliberately absent. There is no public
total-frame product, so long durations do not require a non-C89 integer type.

## fvm_swf_sound_voice_open

Arguments: required owning context, zero-based root StartSound ordinal, required
[FVM_SWFSoundVoice](../src/swf/sound_voice.cpp#L12)** destination. The destination is cleared before all other
validation. Do not pass storage containing a live handle without first closing
it. On success ownership of one native snapshot passes to the caller.

Open validates the complete root control stream and selected unique DefineSound.
SyncStop is not a sample-producing voice and returns UNSUPPORTED. For other
controls no-multiple is copied only: output state is neither queried nor changed.
Fixed-clock in/out boundaries, explicit zero loop count and envelope policies
match the full renderer. Source samples are decoded once; envelope positions
must strictly increase. Unsupported codecs stay UNSUPPORTED even for zero plays
or empty intervals. ADPCM preflight validates the full declared compressed stream.
Zero plays retain geometry but allocate no source PCM and finish immediately.

The combined source PCM and copied [FVM_SWFSoundEnvelope](../include/flashvm/swf_sound.h#L56) payload must fit the
runtime's memory_limit_bytes, or [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15) when unset. Repeated output
is not reserved. The fixed handle allocation, allocator bookkeeping, canonical
input and caller output buffers are separate. The source interval itself still
must fit: this is not a compressed packet streaming decoder or disk-backed cache.
Arithmetic is bounded before allocation. All partial native storage is released
on failure, and the handle remains null.

Results: OK publishes one snapshot, including valid empty/zero-play snapshots;
NOT_FOUND propagates missing ordinal/resource; UNSUPPORTED covers no loaded SWF,
SyncStop, unsupported codec or host sample representation; INVALID_ARGUMENT covers
null context/destination, lifecycle phase, malformed controls/resources/ranges
and non-increasing envelopes; OUT_OF_MEMORY covers budget or native allocation
failure. Error precedence follows metadata validation, then control/range/codec,
then preparation. No output callback or channel binding occurs on any result.

## fvm_swf_sound_voice_read

Arguments: live snapshot, requested sample frames (size_t), caller-owned short
buffer, capacity in short channel values, required size_t* written. Stereo needs
twice the capacity per requested frame. `written` counts channel values, not
frames. A valid written destination is set to zero first.

Validate capacity for the entire request, even when fewer frames remain. A
nonempty request needs a non-null PCM destination. Invalid arguments return
INVALID_ARGUMENT without advancing progress or changing PCM; a null written
pointer returns INVALID_ARGUMENT without modifying snapshot state. A zero-frame
request accepts a null PCM buffer and returns OK with zero written even at EOF.
A nonempty request at EOF returns NOT_FOUND, writes no samples and advances
nothing. An otherwise valid read returns OK with one through requested frames;
its final chunk may be shorter. Reaching EOF is reflected immediately by info,
while NOT_FOUND is returned on the next nonempty read.

After validation, read cannot fail: it performs no allocations, source parsing,
VM access, GC or host callbacks. It advances play/frame and cached envelope
position while filling the caller buffer. Envelope time continues across chunks
and loops, with the same linear interpolation, mono gain averaging and S16
truncation/saturation as full rendering. Per-play source indexing is retained
separately, avoiding a potentially overflowing total-frame size_t product.
Output is native-endian interleaved S16 at the captured nominal source rate.
There is no implicit rewind, seek, resampling or retry after failed host output.
If the host write fails after a read, the snapshot has already advanced; retain
that chunk to retry explicitly or abandon the voice.

## fvm_swf_sound_voice_info

Arguments: live snapshot and required [FVM_SWFSoundVoiceInfo](../include/flashvm/swf_sound.h#L83)* with exact
struct_size. OK copies geometry/progress without allocation, callbacks, GC or
advancement. INVALID_ARGUMENT rejects null handle/output or incorrect size;
output remains unchanged. Info remains available after the originating VM dies.

## fvm_swf_sound_voice_close

Argument: live snapshot or null. Always returns OK for valid usage, releasing
only native copied source/envelope storage. Null is a successful no-op. No VM,
output stop or finalizer callback is entered. It does not close audio channels
or host devices. The handle is invalid immediately after a non-null close.

## Русский контракт

[FVM_SWFSoundVoice](../src/swf/sound_voice.cpp#L12) — явная host-owned native-копия диапазона PCM и огибающей одного
корневого StartSound. Источник декодируется один раз, повторы не копируются целиком.
Чтение выдаёт caller-sized native S16; StreamSoundBlock, автоматический clock,
смешивание и управление stop/no-multiple в этот API не входят. SyncStop не имеет
сэмплов и при open возвращает UNSUPPORTED; no-multiple только сохраняется в flags.
Для синхронизации выходов используется отдельный event_submit.

open(context, ordinal, out) выполняется на VM-потоке вне destruction/GC phases,
проверяет текущий FWS/CWS целиком и уникальный ID, обнуляет out до других проверок.
Владение успешным handle получает host. Диапазоны, ноль повторов, строго
возрастающие точки и математическая огибающая совпадают с full renderer.
Пустой диапазон/ноль повторов дают валидный finished handle; даже тогда кодек
должен поддерживаться и ADPCM проверяется полностью. Лимит временного payload
учитывает только source PCM + копии точек, без размножения на play_count.
Wrapper, allocator metadata, вход SWF и caller buffers — отдельно. Большой сам
источник всё равно может не поместиться; это не packet streaming decoder.

OK — snapshot создан; NOT_FOUND — отсутствует событие/ресурс; UNSUPPORTED — нет
SWF, stop-control/декодера/host S16; INVALID_ARGUMENT — аргументы/фаза/ABI,
повреждение, диапазон или порядок точек; OUT_OF_MEMORY — лимит/выделение. При
ошибке partial native state освобождается, out остаётся null, вывода/binding нет.

read(voice, frames, pcm, capacity_values, written_values) сначала обнуляет written,
проверяет полную capacity запрошенных кадров и указатели. Ошибка не меняет PCM и
позицию. Нулевой запрос разрешён с pcm=null и возвращает OK даже на EOF. Непустой
запрос на EOF — NOT_FOUND/0; последний успешный chunk может быть короче запроса.
EOF сразу виден через info, NOT_FOUND приходит на следующем непустом read.
После проверки read не может отказать, не выделяет память, не читает VM/SWF,
не вызывает host/GC. Огибающая продолжается между chunks и повторами; выход
идентичен полному renderer. Если последующий host write отказал, позиция уже
продвинута: сохраните этот chunk для явного retry либо закройте голос.

info(voice, out) требует точный struct_size и копирует геометрию/позицию без
изменений; INVALID_ARGUMENT не меняет out. source_first — переведённый кадр
источника, frames_per_play — длина исходного интервала даже при zero plays.
play_index/frame_in_play — следующий кадр; после непустого конца play_index равен
play_count, frame_in_play=0. Пустой голос finished уже при play_index=0.

[GC:] Snapshot не удерживает context/runtime/managed roots/borrowed bytes. Он
живёт после GC/reload/уничтожения VM, пока host не вызовет close ровно один раз.
close(null) — OK; ненулевой close освобождает native state без VM/stop callbacks.
Закрытые handles и конкурентный доступ недопустимы. После open возможна передача
другому потоку с корректной host-синхронизацией; операции одного handle должны
быть сериализованы. Буферы/счётчики не должны alias handle/internal/SWF storage.

## Example and implementation

The [swf_audio tutorial](../examples/multimedia/swf_audio/README.md) adds
`INPUT.swf EVENT_ORDINAL OUTPUT.pcm --stream-event`: prepare once, read bounded
chunks, submit them through actual host stdio and stop the output explicitly.
A SyncStop control is rejected by this mode; use --event to apply synchronization.

[`fvm_swf_sound_gain`](../src/swf/sound_style.hpp#L31) and [`fvm_swf_sound_position`](../src/swf/sound_style.hpp#L10) share arithmetic between the
full renderer and snapshots. Tests compare five chunk partitions over SWF1–8
PCM and all four ADPCM code widths, including block transitions. Native allocation
failure tests drain 65535 plays after VM destruction with allocations disabled.

Prepared chunks can be summed through [`fvm_audio_mix_s16`](../include/flashvm/multimedia.h#L179) without allocating:
[block mixer contract](AUDIO_MIXING.md), [swf_mix tutorial](../examples/multimedia/swf_mix/README.md).
This does not apply automatic event scheduling or synchronization to snapshots.
## Independent mixer integration

Prepared voices now also back the optional host-owned
[StartSound mixer](SWF_SOUND_MIXER.md), which applies mixer-local stop/no-multiple
and combines incremental reads. Standalone voices retain their existing
independent behavior. Это отдельный владелец/пространство управления, а не
автоматическая смена поведения старых voice/channel API.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:179](../include/flashvm/multimedia.h#L179) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:196](../include/flashvm/swf_sound.h#L196) |
| `fvm_swf_sound_gain` | function | [src/swf/sound_style.hpp:31](../src/swf/sound_style.hpp#L31) |
| `fvm_swf_sound_position` | function | [src/swf/sound_style.hpp:10](../src/swf/sound_style.hpp#L10) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:162](../include/flashvm/swf_sound.h#L162) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:142](../include/flashvm/swf_sound.h#L142) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:100](../include/flashvm/swf_sound.h#L100) |
| `fvm_swf_sound_voice_read` | function | [include/flashvm/swf_sound.h:158](../include/flashvm/swf_sound.h#L158) |
| `FVM_SWFSoundEnvelope` | struct | [include/flashvm/swf_sound.h:56](../include/flashvm/swf_sound.h#L56) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:12](../src/swf/sound_voice.cpp#L12) |
| `FVM_SWFSoundVoiceInfo` | struct | [include/flashvm/swf_sound.h:83](../include/flashvm/swf_sound.h#L83) |
<!-- source-links:end -->
