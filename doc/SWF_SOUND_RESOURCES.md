# SWF DefineSound inspection and explicit PCM output

## Scope / Область реализации

The C89 API in [swf_sound.h](../include/flashvm/swf_sound.h) reads root-level
DefineSound tags from the canonical SWF currently owned by a context after
[`fvm_load_memory`](../include/flashvm/flashvm.h#L72). FWS and CWS use the same decoded view. It can count/enumerate
root definitions, inspect a selected ID, extract a caller-selected 16-bit PCM
range and submit that range to a registered audio channel. NO_MEDIA needs no
FFmpeg or sound device for inspection/conversion. A separately enabled
[ALSA provider](ALSA_AUDIO_OUTPUT.md) can receive the decoded PCM explicitly.

This is an explicit host resource API, not automatic SWF audio playback.
MovieClip timeline import still rejects unsupported sound tags; loading an
independent clip/resource graph is not a substitute for [`fvm_load_memory`](../include/flashvm/flashvm.h#L72) here.
StartSound/StartSound2, streaming tags, Sound objects, linkage names, loops,
envelopes, in/out sample controls, mixing and scheduled clocks remain open.
Compressed codecs and eight-bit PCM are inspectable but return UNSUPPORTED
from this decoder. The view reports their nominal header flags, not actual
codec-probed rate/channels/width. It does not validate compressed bitstreams or
all codec/version restrictions. Duplicate selected sound IDs are rejected;
this is not validation of the entire SWF character dictionary across all kinds.

Wire layout reference: Adobe's [SWF specification, mirrored original PDF](https://open-flash.github.io/mirrors/swf-spec-19.pdf),
DefineSound and audio coding formats. No original Flash Player acceptance is
claimed. Decode supports SWF ADPCM format 1 and native-order legacy PCM
format 0 / SWF4+ little-endian PCM format 3 with 8- or 16-bit samples. Legacy
format 0 preserves the original format's platform byte-order dependency.
Format 3 converts little-endian bytes explicitly; signed limits are decoded
without implementation-defined unsigned-to-signed narrowing. Rate code zero is
represented as the nominal integer 5512 Hz, not a fractional resampling clock.

## Unsigned eight-bit PCM / восьмибитный PCM

Formats 0 and 3 share the same unsigned eight-bit representation. Conversion
uses `(byte - 128) * 256`, so 0 maps to -32768, 128 to silence and 255 to 32512.
Stereo retains left/right interleaving. The same allocation-free, transactional
range/capacity rules apply. Output always uses native S16; header metadata still
reports the original eight-bit width. This explicit offset-binary conversion is
verified against FFmpeg U8-to-S16, not certified against original Flash Player
amplitude/volume behavior. No dithering, scaling policy or resampling is added.

Восьмибитные форматы 0/3 совпадают; преобразование `(байт - 128) * 256` сохраняет
все 256 значений в старших битах S16. 128 — тишина. Метаданные сохраняют исходную
ширину 8 бит; выход — native S16. Проверка FFmpeg подтверждает преобразование,
но не означает сверку громкости с оригинальным Flash Player.

Run `python3 scripts/check_swf_u8_ffmpeg.py build-crash/flashvm_examples_multimedia_swf_audio_main`
for the independent exhaustive conversion oracle.

## SWF ADPCM / дифференциальное аудио

[`FVM_SWF_SOUND_ADPCM`](../include/flashvm/swf_sound.h#L10) identifies format 1. Read/submit decode two-, three-, four-
and five-bit codes to signed-16 interleaved PCM. Each block contains its initial
sample frame followed by up to 4095 differential frames. Channel state is
independent; predictors saturate to signed-16 and step indices to 0..88. Blocks
are not byte-aligned. The nominal header width does not change ADPCM output.

Every read validates all declared sample frames before writing any destination
value, including zero-length reads. Missing width/header/code bits return
INVALID_ARGUMENT; a final partial block is permitted. Bytes/bits after the
declared sample count are ignored (padding or extra encoded samples). Even an
empty sound requires the two-bit code-width field. Metadata lookup alone does
not validate the compressed body. No allocation or persistent decoder cache is
introduced. Reads scan from the beginning twice when producing samples; repeated
small slices are therefore not a real-time streaming interface. Submit adds its
existing preflight validation and bounded PCM allocation.

ADPCM format 1: 2–5 бит на код, независимые каналы, первый кадр блока хранится
целиком; далее до 4095 дифференциальных кадров без выравнивания блоков на байт.
Read сначала проверяет все заявленные кадры, затем публикует диапазон, поэтому
ошибка в конце потока не меняет начало буфера. Нулевой диапазон тоже проверяет
битстрим. Неполный последний блок допустим; лишние биты/байты после sample_count
игнорируются. Даже пустому звуку нужны два бита ширины. Выход всегда S16,
независимо от номинального header width. Память не выделяется, состояния между
вызовами нет; повторные короткие чтения повторно обходят поток с начала.

Independent oracle: `python3 scripts/check_swf_adpcm_ffmpeg.py
build-crash/flashvm_examples_multimedia_swf_audio_main` uses system FFmpeg only
as a test decoder through separately constructed FLV packets. Runtime ADPCM
requires neither FFmpeg nor a multimedia SDK.

## Ownership, timing and collector rules

Use the context's owning VM thread. Queries and conversion reject destruction,
collector collection and release phases with INVALID_ARGUMENT. They are allowed
in an ordinary native callback, allocate nothing and invoke no user code.
`FVM_SWFSoundInfo.data` borrows complete SoundData bytes, including any codec
prefixes: for example, MP3 seek bytes are not stripped. The view and pointers
remain valid through ordinary GC and failed reload, but become invalid on a
successful context reload or destruction. Do not modify borrowed bytes or use
a caller destination overlapping canonical SWF storage.

[GC:] Canonical SWF bytes are context-owned native storage, not a managed string
or temporary root. Sound inspection introduces no managed graph edges or roots.
[`fvm_swf_sound_submit`](../include/flashvm/swf_sound.h#L38) converts to its own native PCM copy before host output.
The channel callback barrier blocks explicit GC and registry mutation; a callback
which successfully reloads the context cannot invalidate that submitted copy.
No VM locals, call arguments or per-frame debug snapshots are added by this API.

Root scans validate every encountered DefineSound header and exact PCM payload
geometry before publishing results. Unknown root tags are skipped by their
already bounded lengths; nested sprite sound definitions are not searched.
The selected ID must be unique among root DefineSound occurrences. Count counts
occurrences and does not reject duplicate IDs by itself. Enumeration resolves
the selected ordinal and then checks uniqueness of its ID; it can therefore
fail on a duplicate even though counting succeeded. It does not assign names.

The current implementation scans canonical root tags per query; ordinal lookup
and submit may scan more than once. It is an explicit bounded inspection/output
path, not a cached real-time mixer. Hosts choose chunk sizes and playback clocks.
Zero-length supported ranges perform validation but never open/activate output.

## FVM_SWFSoundInfo and codec constants

Set struct_size to sizeof([FVM_SWFSoundInfo](../include/flashvm/swf_sound.h#L13)) before info/at. Other fields are
published only on success: sound_id is the UI16 key; format is the SoundFormat
nibble; sample_rate is one of 5512/11025/22050/44100; bits_per_sample is the
header 8/16 flag; channels is header mono/stereo; sample_count counts frames,
not channel values; data/data_size are the complete borrowed SoundData range.
For compressed codecs these header values can be ignored by the codec.
[`FVM_SWF_SOUND_PCM_NATIVE`](../include/flashvm/swf_sound.h#L9) and [`FVM_SWF_SOUND_PCM_LE`](../include/flashvm/swf_sound.h#L11) identify the two uncompressed
formats; neither implies an eight-bit decoder is available. The struct is a
borrowed view and is not graph-serializable device/resource ownership.

## fvm_swf_sound_count

**Parameters:** required context and required size_t output. **Results:** OK
copies the number of validated root DefineSound occurrences, including zero.
UNSUPPORTED means no canonical SWF is loaded or the host byte representation
cannot support this wire format. INVALID_ARGUMENT rejects invalid lifecycle,
null output or malformed sound header/PCM geometry. Failure leaves the caller's
count unchanged. No allocation, callback, device operation or ownership transfer.

## fvm_swf_sound_at

**Parameters:** required context, zero-based size_t ordinal in file order, and
required exact-size info destination. **Results:** OK publishes that definition;
NOT_FOUND for an ordinal outside the root sound count; INVALID_ARGUMENT for
invalid destination/lifecycle/data or duplicate selected ID; UNSUPPORTED for no
loaded canonical SWF/unsupported host representation. Every failure leaves the
entire output unchanged. No allocation or user callback; returned bytes borrow
the context as described above.

## fvm_swf_sound_info

**Parameters:** required context, nonzero UI16 sound ID (1..65535), required
exact-size info destination. **Results:** OK publishes a unique matching root
sound; NOT_FOUND if absent; INVALID_ARGUMENT for invalid ID/destination/lifecycle,
malformed sound headers, exact PCM size disagreement or duplicate selected ID;
UNSUPPORTED when no canonical movie is loaded or wire representation is not
supported. Format-3 headers before SWF4 are rejected. All failures preserve
output fields. Other codecs remain opaque; this function does not decode them.
No allocation, callback or implicit device opening.

## fvm_swf_sound_read_pcm

**Parameters, in order:** required context; selected ID; first sample-frame
index; size_t number of frames; caller-owned short destination; capacity in
short channel values; required size_t written destination. Stereo needs twice
as many destination values as frames. A null PCM destination is accepted only
for a zero-frame range; an empty range may start exactly at sample_count.

**Results:** OK publishes written channel values and native signed-16 PCM.
UNSUPPORTED rejects other compressed codecs and incompatible host short
representation. INVALID_ARGUMENT rejects out-of-range intervals, insufficient
capacity, nonempty null output and invalid lookup/lifecycle inputs. NOT_FOUND
propagates a missing sound. A valid written pointer is set to zero before other
validation. Failure writes no PCM samples. A null written pointer is itself
invalid. There is no allocation, alias adoption, callback, GC or scheduling.
16-bit host representation requires two-byte short with maximum 32767.

## fvm_swf_sound_submit

**Parameters, in order:** required owning context; selected ID; live channel
owned by that same context; first sample frame; size_t frame count. It uses the
same supported PCM formats and range rules as read_pcm. Rejects channel mutation
reentry through output callbacks as INVALID_ARGUMENT before decoding.

**Results:** lookup/range/codec errors propagate without output; OUT_OF_MEMORY
covers temporary native PCM allocation or a decoded range larger than the
configured runtime memory_limit_bytes (otherwise [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15)). The
limit is a temporary-buffer bound, not an additional managed heap charge.
A valid zero-frame range returns OK without activating or calling the output.
For a nonempty range, the host receives one guarded AUDIO frame and its exact
result is returned without replay. Success activates the registered channel;
failure preserves its previous acknowledged state. Source-relative seconds is
first/sample_rate with has_timestamp set; it is not a VM playback clock.

The temporary PCM copy survives successful context reload inside the callback,
but must not be retained after that callback returns. This function does not
create a channel/provider, infer an endpoint, seek an existing decoder or close
host resources. Existing StopSounds/global stopping rules apply afterwards.

## Русский контракт

API читает корневые DefineSound из canonical SWF, явно загруженного через
[`fvm_load_memory`](../include/flashvm/flashvm.h#L72). Count считает вхождения, at перечисляет их с нулевого индекса,
info ищет уникальный ненулевой UI16 ID. Все корневые звуковые заголовки и точный
размер PCM проверяются до публикации; вложенные sprites не обходятся. Дубликат
выбранного ID — ошибка at/info, но count считает оба вхождения. Полная глобальная
уникальность ID по всем типам ресурсов и сжатые битстримы этим API не проверяются.

View содержит номинальные rate/channels/bits и borrowed SoundData, включая
codec-specific префиксы. Данные живут до успешной перезагрузки/уничтожения
контекста; обычный GC и неудачная загрузка view не инвалидируют. Read поддерживает
8/16-bit PCM format 0 (native-endian), format 3 (little-endian, SWF4+) и ADPCM format 1.
Остальные сжатые форматы пока UNSUPPORTED. Начало/размер диапазона — в sample frames,
capacity/written — в short channel values. При ошибке PCM не пишется, written
обнуляется; при ошибке count/info/at их назначения остаются прежними.

Все операции — на VM-потоке, не в destruction/GC collection/release. Queries/read
не выделяют память и не вызывают host код. Submit требует канал того же контекста,
делает ограниченную native-копию PCM и ровно один защищённый output callback.
Это позволяет callback перезагрузить SWF без use-after-free отправленного кадра.
Временный буфер ограничен memory_limit_bytes либо [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15); это не
managed GC accounting. Пустой диапазон не активирует выход. Параметры/статусы
каждой функции выше одинаковы для RU/EN. Автоматический StartSound, streaming,
микшер, циклы, огибающие, AS Sound и полная совместимость Flash остаются открытыми.

## Validation checkpoint v0.548

Minimal full suite: 1141 passed, one existing named Unix endpoint skip (1142
entries). Native static ALSA 1.2.12: four targeted tests passed, including actual
DefineSound conversion → real null PCM → StopSounds. The same four passed with
ASAN/UBSAN on FlashVM; external ALSA/zlib were not instrumented and LSAN remains
disabled for this runner. Three targeted JIT-profile tests passed, with requested
interpreter/JIT execution modes for the StopSounds step. The native PCM decoder
itself is ordinary C++98, not newly generated JIT code.

The C89 tutorial exported independent FWS and CWS fixtures byte-for-byte and
correctly left output absent for unsupported eight-bit and valid empty sounds.
Negative-wrapped/invalid IDs were rejected before file access. All 90 catalog
entries, source links and 117 multimedia/tutorial source files passed checks.
This evidence does not certify full SWF sound semantics or audible hardware.

## Tutorial and evidence

[swf_audio](../examples/multimedia/swf_audio/README.md) enumerates a supplied
movie and explicitly exports one sound to raw PCM through a real file callback.
The tests use independent wire fixtures, signed limits, mono/stereo, SWF1–8,
long tag headers, CWS ownership, duplicate/malformed definitions, failed/successful
reload, allocation faults and a real static ALSA null-PCM output pipeline.
Physical audible playback and original Flash Player equivalence remain separate.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `FVM_SWF_SOUND_ADPCM` | macro / define | [include/flashvm/swf_sound.h:10](../include/flashvm/swf_sound.h#L10) |
| `fvm_swf_sound_at` | function | [include/flashvm/swf_sound.h:29](../include/flashvm/swf_sound.h#L29) |
| `fvm_swf_sound_count` | function | [include/flashvm/swf_sound.h:27](../include/flashvm/swf_sound.h#L27) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:31](../include/flashvm/swf_sound.h#L31) |
| `FVM_SWF_SOUND_PCM_LE` | macro / define | [include/flashvm/swf_sound.h:11](../include/flashvm/swf_sound.h#L11) |
| `FVM_SWF_SOUND_PCM_NATIVE` | macro / define | [include/flashvm/swf_sound.h:9](../include/flashvm/swf_sound.h#L9) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:34](../include/flashvm/swf_sound.h#L34) |
| `fvm_swf_sound_submit` | function | [include/flashvm/swf_sound.h:38](../include/flashvm/swf_sound.h#L38) |
| `FVM_SWFSoundInfo` | struct | [include/flashvm/swf_sound.h:13](../include/flashvm/swf_sound.h#L13) |
<!-- source-links:end -->
