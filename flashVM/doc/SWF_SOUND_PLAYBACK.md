# Explicit StartSound voice output / Явный вывод StartSound

## Scope

[`fvm_swf_sound_event_submit`](../include/flashvm/swf_sound.h#L97) connects one root StartSound control to a dedicated
[`FVM_AudioChannel`](../src/multimedia/audio/channel.hpp#L6). It renders supported PCM/ADPCM, source boundaries, repetitions
and linear envelopes, then calls the real registered output once. Stop and
no-multiple inspect all same-ID bound voices in the owning context. This is an explicit
single-voice building block: hosts select events and channels, schedule time and
allocate separate channels for overlapping voices. MovieClip frame advancement
does not invoke it automatically. StartSound2, button/streaming audio, mixing,
resampling and compressed codecs other than ADPCM remain open. A host callback
must consume/copy the complete buffer synchronously; no hidden worker is created.

## fvm_swf_sound_event_submit

Arguments, in order: owning context; zero-based root event ordinal in the current
canonical loaded SWF; live channel owned by that context. The channel must be
dedicated to this resource. An unbound channel must be inactive. A successful
nonempty event write binds its sound ID until the channel is closed; stopping
or reloading SWF does not erase this binding. A different sound ID is rejected
on this channel, including by [fvm_swf_sound_submit](../include/flashvm/swf_sound.h#L100). Plain host writes remain
available; the host must use them only for this dedicated resource. Closing and
re-registering permits a new identity. There are no managed objects or GC roots
in the binding, and it is not serialized across contexts.

The operation requires the VM thread, outside context destruction, collector
collection/release and audio callbacks. Foreign channels, callback reentry,
invalid phases and incorrect resource binding return INVALID_ARGUMENT. Null or
stale handles are not made safe by this API; as with other channel operations,
a closed/context-destroyed handle must never be used. The runtime and host
userdata must outlive the channel/context cleanup.

Processing order:

1. Validate the complete root event stream and resolve the selected unique
   DefineSound using [fvm_swf_sound_event_at](../include/flashvm/swf_sound.h#L58). Preserve missing/malformed errors.
2. Validate resource/channel identity. If SyncStop is set, stop every active
   bound same-ID voice in this context; stop wins over no-multiple and rendering
   styles. Attempt all matches in registration order, preserving the first host
   error and retaining only failed voices as active. No PCM is decoded/allocated.
   No matching active voice means OK without a callback; the chosen unbound
   target is not bound by a stop.
3. SyncNoMultiple returns OK without allocation/output if any registered bound
   same-ID voice in this context is active. The chosen target keeps its state
   and an unbound target is not bound by suppression. Other IDs, unbound raw
   outputs and other contexts do not participate. An ordinary new start still
   requires an inactive target: otherwise INVALID_ARGUMENT. Hosts allocate
   separate channels for overlapping instances.
4. For an inactive channel, validate/render the full styled voice before any host
   callback. Empty valid voices return OK without activation or binding. A
   nonempty voice is submitted once; success commits the binding/active state.

The callback receives native interleaved S16 at the source's nominal rate and
channel count. `samples` includes every repetition. `seconds` is zero with a
valid timestamp: it is the new voice's elapsed origin, not the SWF frame clock
or the source in-point. Native copied data remain valid through a successful
[fvm_load_memory](../include/flashvm/flashvm.h#L72) inside the output callback. Normal audio callback barriers block
GC, nested audio mutation and context destruction. The buffer is freed after
return; a backend that needs it later must copy it. There is no hidden retry.

Return values: OK means rendered/accepted, suppressed duplicate, inactive stop
or valid empty output; NOT_FOUND means absent event/resource; UNSUPPORTED means
no loaded SWF or an unsupported decoder/host sample representation for rendering;
INVALID_ARGUMENT includes bad ranges, incompatible channel state, malformed
records or non-increasing envelope positions; OUT_OF_MEMORY covers native
preparation failure or configured temporary size limit. A host write/stop error
is returned unchanged. Failed preparation performs no callback and preserves
channel identity/active state. A failed write does not commit a new binding;
its native side effects cannot be rolled back. A failed resource stop retains failed voices as active
for retry; successfully stopped peers remain inactive with their bindings. Successful output remains logically active until
explicit stop/close/StopSounds, even if physical playback has already completed.

## Rendering policy and limits

InPoint/OutPoint use [FVM_SWF_SOUND_CONTROL_RATE](../include/flashvm/swf_sound.h#L27), a fixed 44100 Hz control clock. Each converts to a source
frame by floor(position * nominal_rate / 44100), using overflow-safe integer
arithmetic. The source interval begins at the converted InPoint or zero. With
explicit OutPoint the final included frame is its converted position; otherwise the end is the sound's declared length.
The converted OutPoint must be smaller than sample_count, and the start must not exceed the
exclusive end. Starting exactly at sample_count without an explicit OutPoint
produces an empty voice. Unsupported empty codecs still return UNSUPPORTED.
Absent loop count renders once; explicit zero produces no voice. LoopCount is
the total number of plays, not extra repetitions. Zero-play rendering still
validates range, codec and envelope ordering but does not allocate source/output PCM. Source samples are decoded once.

Envelope points must have strictly increasing positions for rendering (metadata
inspection still preserves unordered/duplicate points). No points means unity.
Before the first point its gain is held; between points gains interpolate
linearly; after the last point its gain is held. Envelope time starts at elapsed
voice frame zero and continues across repeats. It is not reset for each loop or
advanced by the in-point. Its fixed clock is [FVM_SWF_SOUND_ENVELOPE_RATE](../include/flashvm/swf_sound.h#L28); mapping
uses the nominal source rate, including 5512 Hz for rate code zero. Stereo uses
independent gains; mono averages the two gains before multiplication. Division
by [FVM_SWF_SOUND_ENVELOPE_UNITY](../include/flashvm/swf_sound.h#L26) produces signed-S16 values truncated toward zero,
with defensive saturation. There is no dithering or sample-rate conversion.
Fixed-clock controls and zero-loop semantics were cross-checked with
[pinned Ruffle EventSoundStream](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/backend/audio/mixer.rs#L796).
These explicit rendering policies are fixture-tested; original Flash Player
volume/loop/envelope equivalence has not been certified, particularly at 5.5 kHz.

Temporary storage includes the decoded source interval, the full repeated output
and copied [FVM_SWFSoundEnvelope](../include/flashvm/swf_sound.h#L48) records. Their combined payload must fit
memory_limit_bytes, or the default [FVM_SWF_DEFAULT_LIMIT](../include/flashvm/swf.h#L15) when unset. Arithmetic
is checked before products/allocation. Allocator bookkeeping, channel wrappers,
canonical input and the host's output copy are separate. Allocation occurs
outside managed GC. Large/long events can return OUT_OF_MEMORY even though raw
[fvm_swf_sound_submit](../include/flashvm/swf_sound.h#L100) can export the source in smaller chunks. No persistent
cache, streaming decoder or automatic mixing buffer is added.

## Русский контракт

`fvm_swf_sound_event_submit(context, ordinal, channel)` применяет корневой
StartSound к явно выбранному голосовому каналу: PCM/ADPCM, InPoint/OutPoint,
повторы и огибающую. Требуется канал того же context, VM-поток, отсутствие
GC/destruction/audio callback. ordinal начинается с нуля и относится к текущему
canonical FWS/CWS. Первый успешный непустой event write закрепляет sound ID за
каналом до close. Другой ID на этом канале — INVALID_ARGUMENT, включая обычный
[fvm_swf_sound_submit](../include/flashvm/swf_sound.h#L100). Stop/reload привязку не снимают; незакреплённый active-канал
нельзя присвоить событию. Пользовательские write остаются доступны под
ответственность host: этот канал должен обслуживать только выделенный ресурс.

Сначала полностью проверяются события и ссылка на ресурс. SyncStop имеет
приоритет, пытается остановить все active bound same-ID голоса контекста без декодирования
PCM. После ошибки продолжает, возвращает первую в порядке регистрации; только
неудачно остановленные остаются active для retry. No-multiple пропускает запуск
с OK при любом active bound same-ID голосе этого контекста, не закрепляя пустой
target. Другие ID, unbound host outputs и контексты не участвуют. Обычный start
требует inactive target, иначе INVALID_ARGUMENT. Для overlap нужны разные каналы.

InPoint/OutPoint используют clock 44100 Hz и переводятся в кадры источника как
floor(position * nominal_rate / 44100), без переполнения. InPoint — начало либо 0,
OutPoint — включительно последний переведённый кадр, без него конец равен sample_count. Выход за длину/обратный диапазон — INVALID_ARGUMENT. Начало
ровно в конце без OutPoint даёт пустой звук. LoopCount — общее число проходов,
отсутствующий count означает один проход, явный ноль — пустой голос.
Нулевые повторы проверяют диапазон/кодек/порядок точек, без source/output PCM allocations. Огибающие требуют строго
возрастающих позиций; отсутствие точек — unity. До/после крайних точек gain
удерживается, между ними — линейная интерполяция. Время начинается с нуля нового
голоса и продолжается через повторы, без сдвига на InPoint и сброса на каждом
цикле. Частота огибающей 44100 Hz; источник использует nominal rate. Mono
усредняет левый/правый gain, stereo применяет отдельно. S16 округляется к нулю,
с защитным ограничением. Эквивалентность громкости/границ с Flash Player ещё
не сертифицирована; эти правила являются явно проверенным контрактом renderer.

До callback готовится полная native-копия. Лимит временной памяти учитывает
исходный диапазон, повторённый выход и точки огибающей; это не лимит всей памяти
процесса. OOM/ошибка подготовки не вызывают host и не меняют канал. Пустой голос
не активирует/не закрепляет канал. Неподдерживаемый кодек остаётся UNSUPPORTED
даже для пустого диапазона. На успешном непустом запуске ровно один guarded
write; на ошибке write новая привязка не сохраняется, побочные эффекты host не
откатываются. Ошибка stop оставляет active для повтора. OK также обозначает
пропущенный duplicate, неактивный stop или пустой голос. NOT_FOUND — нет
события/ресурса; INVALID_ARGUMENT — неверные аргументы, диапазоны, состояние,
повреждение или порядок точек; OUT_OF_MEMORY — размер/выделение; UNSUPPORTED —
нет SWF/декодера; ошибки host возвращаются без изменения.

Буфер borrowed только до возврата callback; userdata живёт до очистки канала/
контекста. Reload внутри callback безопасен для подготовленного PCM. [GC:]
Действует обычный native barrier; GC и повторная мутация audio запрещены.
После успешного write канал считается active до явного stop/close/StopSounds,
а не до автоматического определения конца физического воспроизведения.
Timestamp — elapsed origin 0, не clock кадра SWF. Автоматическая интеграция с
MovieClip, смешивание, планировщик, stream/button/StartSound2 ещё не реализованы.

## Example and implementation

[swf_audio tutorial](../examples/multimedia/swf_audio/README.md) supports
`INPUT.swf EVENT_ORDINAL OUTPUT.pcm --event`. It writes an actual raw S16 file,
not audible output. The ordinary SOUND_ID mode still exports unstyled source
chunks. See the [resource/metadata contracts](SWF_SOUND_RESOURCES.md),
[channels](AUDIO_CHANNELS.md) and [ALSA bridge](ALSA_AUDIO_OUTPUT.md).
[swf_polyphony](../examples/multimedia/swf_polyphony/README.md) demonstrates
two overlapping resource voices, global no-multiple, stop-by-ID and preservation
of a different resource through real independent stdio outputs.
Implementation is split between [`fvm_swf_sound_event_submit`](../include/flashvm/swf_sound.h#L97) (guarded native
output) and [`fvm_swf_sound_render`](../src/swf/sound_render.cpp#L10) (bounded preparation).

## Incremental alternative

For long repetition counts, use an independent [`FVM_SWFSoundVoice`](../src/swf/sound_voice.cpp#L9) snapshot:
[SWF_SOUND_VOICES.md](SWF_SOUND_VOICES.md). It retains one source interval and
produces bounded caller buffers with identical gain/loop arithmetic. Snapshot
reads do not apply channel synchronization; hosts drive their own output and
stop policy. The full event-submit budget/atomic single-callback policy above
is unchanged.

Для длинных повторов подготовленный snapshot хранит источник один раз и читает
порциями. Полный event_submit сохраняет прежний бюджет/один callback. Snapshot
не исполняет stop/no-multiple и не включает автоматический MovieClip clock.

The separate caller-driven [`fvm_audio_mix_s16`](../include/flashvm/multimedia.h#L131) block helper is now available:
[AUDIO_MIXING.md](AUDIO_MIXING.md). It does not connect this event API to an
automatic MovieClip clock or mix bound channels implicitly.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:131](../include/flashvm/multimedia.h#L131) |
| `FVM_AudioChannel` | struct | [src/multimedia/audio/channel.hpp:6](../src/multimedia/audio/channel.hpp#L6) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `FVM_SWF_DEFAULT_LIMIT` | macro / define | [include/flashvm/swf.h:15](../include/flashvm/swf.h#L15) |
| `FVM_SWF_SOUND_CONTROL_RATE` | macro / define | [include/flashvm/swf_sound.h:27](../include/flashvm/swf_sound.h#L27) |
| `FVM_SWF_SOUND_ENVELOPE_RATE` | macro / define | [include/flashvm/swf_sound.h:28](../include/flashvm/swf_sound.h#L28) |
| `FVM_SWF_SOUND_ENVELOPE_UNITY` | macro / define | [include/flashvm/swf_sound.h:26](../include/flashvm/swf_sound.h#L26) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:58](../include/flashvm/swf_sound.h#L58) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_render` | function | [src/swf/sound_render.cpp:10](../src/swf/sound_render.cpp#L10) |
| `fvm_swf_sound_submit` | function | [include/flashvm/swf_sound.h:100](../include/flashvm/swf_sound.h#L100) |
| `FVM_SWFSoundEnvelope` | struct | [include/flashvm/swf_sound.h:48](../include/flashvm/swf_sound.h#L48) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:9](../src/swf/sound_voice.cpp#L9) |
<!-- source-links:end -->
