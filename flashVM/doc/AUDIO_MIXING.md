# Native S16 block mixing / Смешивание блоков native S16

## fvm_audio_mix_s16

This portable C89 operation sums time-aligned interleaved native S16 buffers into
caller-owned output. It uses no allocations, VM/context, managed values, GC,
locks, device calls or host callbacks. It works with NO_MEDIA. It is a block
arithmetic helper, not a scheduler: hosts align inputs, advance snapshots, apply
styles/gains, manage individual voices and write the mixed result. Rate/layout
conversion, automatic MovieClip mixing and device playback are separate.

Arguments in order:

| Argument | Contract |
| --- | --- |
| inputs | Contiguous borrowed array of [FVM_MediaFrame](../include/flashvm/multimedia.h#L37) records; null only when input_count is zero. |
| input_count | Zero through [FVM_MEDIA_AUDIO_MIX_MAX_INPUTS](../include/flashvm/multimedia.h#L18) (1024) inclusive. |
| sample_rate | Required nonzero common nominal rate, also used in output metadata. |
| channels | Required nonzero common interleaved channel count. |
| frames | Exact output sample-frame count, including silence after shorter inputs. |
| destination | Caller-owned short storage; null only for a zero-frame request. |
| capacity_values | Accessible short channel values, not frames or bytes. |
| out_frame | Required output view destination; unchanged on any failure. |

Every input must have kind [FVM_MEDIA_AUDIO](../include/flashvm/multimedia.h#L19) and exactly the requested rate/channel
count. The caller must also ensure identical channel ordering: this ABI carries
no speaker-layout mask. Populated views must satisfy [fvm_media_frame_valid](../src/multimedia/common/session.hpp#L15): non-null accessible
native S16 bytes, positive declared samples and enough even-sized storage for all
of them. Extra accessible bytes are ignored. An exhausted input may use samples=0
and size=0 with either null or non-null data; its rate/channels/kind still must
match. Empty records with nonzero size are rejected. Longer inputs contribute
only the first requested frames; shorter inputs are padded with zero. Timestamps,
video fields and other unused audio fields are ignored, with no implicit clock
comparison. A zero input count produces silence of the requested duration.

Capacity and output byte multiplication are checked without overflow. All input
records are validated before any PCM or output metadata is modified, even on a
zero-frame request. Invalid arguments/geometry/counts return INVALID_ARGUMENT;
an incompatible native short/octet representation returns UNSUPPORTED. All
failures preserve destination PCM and the entire output view. Successful calls
return OK and publish a borrowed [FVM_MediaFrame](../include/flashvm/multimedia.h#L37) over destination, with kind AUDIO,
requested rate/channels/frame count and exact initialized byte size. Timestamp/
video fields are zero. A zero-frame result may have null data and must not be
submitted to APIs requiring a populated output frame.

For each channel sample, all voices are accumulated exactly in signed long,
then saturated once to [-32768, 32767]. The 1024-input cap keeps this sum within
every supported signed-long representation. Input ordering cannot alter final
clipping: [32767, 32767, -32768] sums to 32766, rather than clipping an intermediate
sum. There is no normalization, averaging, gain, dither, resampling or automatic
master-volume policy. Apply source envelopes/gains before mixing if needed.
No actual sample conversion occurs beyond final saturation; input and output
both use native-endian signed 16-bit PCM. Unaligned input byte addresses are
accepted through memcpy; destination must be properly short-aligned.

Ownership remains entirely with the caller. Buffers must actually contain their
declared accessible bytes; pointer metadata is not a memory-allocation probe.
Exact-start in-place payload mixing is supported: destination can equal an input
buffer start with sufficient destination capacity. Partial/offset overlap is
not supported and is the caller's responsibility; metadata arrays/out_frame
must not alias PCM storage. The output view can be used synchronously through
[fvm_audio_channel_write](../include/flashvm/multimedia.h#L83) or [fvm_audio_write](../include/flashvm/multimedia.h#L128) until destination is reused. Retained
playback requires the host bridge to copy bytes. Independent calls may run on
separate threads when input/output memory is properly synchronized; concurrent
mutation of any shared buffer/metadata is invalid. This function itself changes
no channel active flags, resource bindings, snapshot progress or VM state.

## Русский контракт

[fvm_audio_mix_s16](../include/flashvm/multimedia.h#L130)(inputs, count, rate, channels, frames, destination,
capacity_values, out_frame) складывает синхронные native S16 blocks в caller
buffer без аллокаций, VM/GC, блокировок, устройств и callbacks. Работает с
NO_MEDIA. Host сам выравнивает время, читает голоса, применяет огибающие/громкость,
управляет их lifecycle и передаёт готовый блок выходному bridge. Планировщика,
конверсии частоты/каналов и автоматической MovieClip интеграции здесь нет.

inputs — непрерывный массив [FVM_MediaFrame](../include/flashvm/multimedia.h#L37); null допустим только при count=0.
count ограничен [FVM_MEDIA_AUDIO_MIX_MAX_INPUTS](../include/flashvm/multimedia.h#L18) (1024). rate/channels должны быть
ненулевыми и одинаковыми у всех input records. Speaker-layout mask в ABI нет: host
также обеспечивает одинаковый порядок каналов. frames — точное число выходных
кадров. capacity_values — число доступных short values. destination=null
разрешён только для frames=0; out_frame обязателен. У populated входов kind=AUDIO,
данные native S16, samples>0 и все заявленные samples помещаются в доступные
чётные bytes. У пустого входа samples=0,size=0, data может быть null; геометрия
всё равно проверяется. Непустой size при samples=0 — ошибка.

Все входы проверяются до изменения результата. INVALID_ARGUMENT означает
ошибочные указатели/размеры/count/rate/layout; UNSUPPORTED — неподдерживаемое
представление short/octet. При любой ошибке PCM и out_frame неизменны. OK
публикует borrowed output view: kind AUDIO, выбранные rate/channels/frames,
точный byte size; timestamp/video поля обнулены. Zero-frame output нельзя
передавать API, требующему непустой кадр. Нулевой count создаёт тишину.

Короткие входы дополняются нулями, длинные обрезаются по requested frames.
Timestamps не сравниваются: синхронизация — обязанность host. Сначала все
каналы суммируются точно в signed long, затем один раз ограничиваются S16;
порядок входов не влияет на клиппинг. Нормализации, averaging, master gain,
resampling/dither нет. Не выровненные input bytes поддержаны через memcpy,
выход должен быть short-aligned.

Владение не передаётся. Exact-start in-place разрешён при достаточной ёмкости;
partial/offset overlap не поддержан и не обнаруживается автоматически. Metadata
не должна alias PCM storage; реальные buffers обязаны содержать заявленные
байты. После возврата out_frame заимствует destination до его повторного
использования. Для отложенного воспроизведения bridge копирует данные.
Независимые вызовы возможны на разных потоках при host-синхронизации общих
буферов. Mixer не меняет active/binding/позиции voices и не исполняет StartSound.

## Voice integration example

[swf_mix](../examples/multimedia/swf_mix/README.md) prepares two independent
[FVM_SWFSoundVoice](../src/swf/sound_voice.cpp#L9) handles, reads equal-sized source blocks, pads the shorter
voice after EOF, mixes them and writes through an actual host FILE callback.
The combined duration is the longest voice, not the sum of voice lengths.
See [incremental voice ownership](SWF_SOUND_VOICES.md),
[output channel rules](AUDIO_CHANNELS.md) and
[replaceable bridges](MULTIMEDIA_BRIDGES.md).

## Reproducible checks

`python3 scripts/check_swf_mix_example.py build-crash/flashvm_examples_multimedia_swf_mix_main`
checks the actual tutorial, exact native bytes, no-argument side effects and
failed native output cleanup. Add `--ffmpeg` to compare against the installed
FFmpeg amix filter with normalization disabled and longest-input duration.
FFmpeg is a test-only independent oracle; FlashVM's mixer calls no CLI or SDK.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:83](../include/flashvm/multimedia.h#L83) |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:130](../include/flashvm/multimedia.h#L130) |
| `fvm_audio_write` | function | [include/flashvm/multimedia.h:128](../include/flashvm/multimedia.h#L128) |
| `FVM_MEDIA_AUDIO` | macro / define | [include/flashvm/multimedia.h:19](../include/flashvm/multimedia.h#L19) |
| `FVM_MEDIA_AUDIO_MIX_MAX_INPUTS` | macro / define | [include/flashvm/multimedia.h:18](../include/flashvm/multimedia.h#L18) |
| `fvm_media_frame_valid` | function | [src/multimedia/common/session.hpp:15](../src/multimedia/common/session.hpp#L15) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:37](../include/flashvm/multimedia.h#L37) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:9](../src/swf/sound_voice.cpp#L9) |
<!-- source-links:end -->
