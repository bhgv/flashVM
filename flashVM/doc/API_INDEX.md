# FlashVM C89/C++98 API index

- `include/flashvm/gc.h`: collection, mode, pinning, introspection.
- `include/flashvm/value.h`: primitive and object values.
- `include/flashvm/table.h`: indexed elements and metatables.
- `include/flashvm/channel.h`: raw channel transport.
- `include/flashvm/transfer.h`: graph send/receive across queue/file/TCP channels.
- `include/flashvm/property.h`: property flags, deletion and enumerable-name snapshots.
- `include/flashvm/function.h`: first-class native/AVM1 functions, invoke/construct.
- `include/flashvm/avm1.h`: actions, rooted trace/URL sinks and borrowed active-target introspection.
- `include/flashvm/stage.h`: host RGBA background selection and query.
- `include/flashvm/movieclip.h`: host-authored clip frames and vector rendering.
- `include/flashvm/standard.h`: standard installation and primitive conversion.
- `include/flashvm/capability.h`: TCP resource authority and imports.

All public headers must remain valid C89 and usable from C++98. No STL types cross the C ABI.

Object graph coverage and limitations: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

`value.h`: ABI 3 counted UTF-8/WTF-8 and UTF-16 creation/query/copy helpers.

[`fvm_movieclip_swf_document_info`](../include/flashvm/movieclip.h#L33) — [SWF document metadata / метаданные SWF](SWF_DOCUMENT_METADATA.md), `include/flashvm/movieclip.h`.

## Runtime and context

Initialize [`FVM_Config`](../include/flashvm/flashvm.h#L51) with [`fvm_config_init`](../include/flashvm/flashvm.h#L62), create the runtime with
[`fvm_runtime_create`](../include/flashvm/flashvm.h#L64), then create its context with [`fvm_context_create`](../include/flashvm/flashvm.h#L68).
Check each returned [`FVM_Result`](../include/flashvm/flashvm.h#L40) before using an output handle. Destroy the
context with [`fvm_context_destroy`](../include/flashvm/flashvm.h#L70) before [`fvm_runtime_destroy`](../include/flashvm/flashvm.h#L66). Each context
owns its managed object graph; do not access one context concurrently from
multiple threads. Worker examples create independent contexts and exchange
values through the documented transport API.

Сначала инициализируйте конфигурацию, затем создайте runtime и context.
Проверяйте результаты до использования выходных handles; уничтожайте context
раньше runtime. Контекст владеет своим графом объектов и используется его
VM-потоком. Точные декларации: [flashvm.h](../include/flashvm/flashvm.h).

## Values and borrowed views

[`FVM_Value`](../include/flashvm/value.h#L24) carries a type tag and its corresponding payload. Use the
`fvm_value_*` helpers instead of leaving cells uninitialized. A borrowed string
or object view does not transfer ownership. Keep backing bytes valid for the
operation that borrows them; use counted string APIs when embedded NUL matters.
Managed objects retained by the host across GC safe points need explicit roots;
see [GC API](GARBAGE_COLLECTION.md#api). Cross-context object transfer uses
serialization/resource interfaces, not copying `FVM_Object *` pointers.

Ячейка [FVM_Value](../include/flashvm/value.h#L24) хранит тег и значение. Заимствованное представление не передаёт
владение; время жизни конкретного результата определяется вызванной функцией.
Для строк со встроенным NUL используйте API с длиной, для объектов через точки
GC — явные корни. Декларации и границы lifetime: [value.h](../include/flashvm/value.h).

## Native calls and metatables

[`fvm_register_function`](../include/flashvm/value.h#L101) installs a named native callback; [`fvm_call`](../include/flashvm/value.h#L138) invokes a
named function and [`fvm_object_call`](../include/flashvm/value.h#L126) invokes a named method with a receiver.
Check the current declarations in [value.h](../include/flashvm/value.h), and
[AVM1 function contracts](AVM1_FUNCTIONS.md) for script invocation. Registration
userdata belongs to the host; a callback must respect the owning VM thread.

[`fvm_table_set_metatable`](../include/flashvm/table.h#L24) attaches a table; [`fvm_meta_register`](../include/flashvm/metamethod.h#L20) registers a
metamethod. The explicit [`fvm_meta_call`](../include/flashvm/meta_call.h#L20) API invokes native __call with owner,
defining metatable and a fresh indexed argument table (user argument zero is at
index zero). This operation has its own callback convention; it does not imply
implicit AVM1 closure dispatch. See [meta_call.h](../include/flashvm/meta_call.h),
[metamethod.h](../include/flashvm/metamethod.h) and the
[annotated meta-call description](../examples/extending/meta_call/README.ru.md).

Именованные native-функции, методы с receiver и явный meta-call — разные
соглашения вызова. Выбирайте API по примеру и не переносите правила аргументов
одного соглашения на другое. Stack callbacks описаны отдельно в [STACK_API.md](STACK_API.md).
Documentation maintenance / Ведение документации: [rules and workflow](DOCUMENTATION_GUIDELINES.md).

Stage policy: [`fvm_stage_get_quality`](../include/flashvm/stage.h#L19), [`fvm_stage_set_quality`](../include/flashvm/stage.h#L22), [`fvm_stage_toggle_quality`](../include/flashvm/stage.h#L25)
— [STAGE_QUALITY.md](STAGE_QUALITY.md), including parameters, statuses, lifetime,
ToggleQuality transitions and renderer limits.
## Multimedia and devices / Мультимедиа и устройства

[Bridge API and detailed contracts](MULTIMEDIA_BRIDGES.md); [public header](../include/flashvm/multimedia.h).
Decoder/capture sessions, synchronous PCM/RGBA output, native input and future sensor providers.
Native/interpreter StopSounds equivalence: [JIT_SOUND.md](JIT_SOUND.md).

Root streaming-sound segments, frame blocks and bounded PCM reads:
[SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md), [MP3 prefix/encoded snapshots](SWF_STREAM_MP3.md), [swf_stream_sound.h](../include/flashvm/swf_stream_sound.h).

Context audio registration and script stopping: [AUDIO_CHANNELS.md](AUDIO_CHANNELS.md).
[`fvm_audio_channel_create`](../include/flashvm/multimedia.h#L82), [`fvm_audio_channel_write`](../include/flashvm/multimedia.h#L84), [`fvm_audio_channel_stop`](../include/flashvm/multimedia.h#L86),
[`fvm_audio_channel_close`](../include/flashvm/multimedia.h#L88), [`fvm_audio_channel_active`](../include/flashvm/multimedia.h#L90) and [`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L92)
provide copied output tables, explicit ownership and shared StopSounds semantics.

Native Linux output: [`fvm_audio_linux_alsa_open`](../include/flashvm/multimedia.h#L77), [`fvm_audio_linux_alsa_close`](../include/flashvm/multimedia.h#L79) —
[static ALSA provider contracts](ALSA_AUDIO_OUTPUT.md).

Loaded SWF audio resource/StartSound inspection and explicit PCM output: [`FVM_SWFSoundInfo`](../include/flashvm/swf_sound.h#L13),
[`fvm_swf_sound_count`](../include/flashvm/swf_sound.h#L86), [`fvm_swf_sound_at`](../include/flashvm/swf_sound.h#L88), [`fvm_swf_sound_info`](../include/flashvm/swf_sound.h#L90),
[`fvm_swf_sound_read_pcm`](../include/flashvm/swf_sound.h#L93), [`fvm_swf_sound_submit`](../include/flashvm/swf_sound.h#L100) —
[SWF_SOUND_RESOURCES.md](SWF_SOUND_RESOURCES.md), [public header](../include/flashvm/swf_sound.h).
Copied timeline metadata: [`FVM_SWFSoundEvent`](../include/flashvm/swf_sound.h#L36), [`FVM_SWFSoundEnvelope`](../include/flashvm/swf_sound.h#L48),
[`fvm_swf_sound_event_count`](../include/flashvm/swf_sound.h#L56), [`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L58), [`fvm_swf_sound_event_envelope`](../include/flashvm/swf_sound.h#L60) —
[StartSound contracts](SWF_SOUND_RESOURCES.md#root-startsound-records--события-startsound).
These inspect controls without automatically scheduling playback.
[`fvm_swf_sound_event_submit`](../include/flashvm/swf_sound.h#L97) applies one bounded styled voice to a dedicated
channel: [playback contract](SWF_SOUND_PLAYBACK.md).

Prepared independent sequential voices: [`FVM_SWFSoundVoice`](../src/swf/sound_voice.cpp#L9), [`FVM_SWFSoundVoiceInfo`](../include/flashvm/swf_sound.h#L63),
[`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L78), [`fvm_swf_sound_voice_read`](../include/flashvm/swf_sound.h#L82), [`fvm_swf_sound_voice_info`](../include/flashvm/swf_sound.h#L80),
[`fvm_swf_sound_voice_close`](../include/flashvm/swf_sound.h#L84) — [incremental voice contracts](SWF_SOUND_VOICES.md).
Source PCM is retained once; snapshot reads allocate nothing and outlive the VM.

Portable bounded S16 summing: [`fvm_audio_mix_s16`](../include/flashvm/multimedia.h#L131), [`FVM_MEDIA_AUDIO_MIX_MAX_INPUTS`](../include/flashvm/multimedia.h#L19) —
[AUDIO_MIXING.md](AUDIO_MIXING.md). Caller-aligned blocks, final-only saturation,
short-tail silence and no allocation/implicit device or clock.

## Speech / Речевые службы

[Detailed TTS/STT API and per-OS preparation](SPEECH_BRIDGES.md); [public speech.h](../include/flashvm/speech.h).
Separate explicit voice and microphone sessions, replaceable native providers, retained UTF-8 transcript query/copy.


Prepared PCM stream snapshots / Независимые индексированные снимки PCM: [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm). Six C89 functions copy/index one validated root PCM segment; indexed reads survive source VM destruction without allocation. Preparation is explicitly budgeted; compressed codecs and automatic MovieClip audio clocks remain unsupported.


Windows x64 backend selection, W^X and OS unwind-table ownership / выбор Windows x64 backend, W^X и время жизни unwind: [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). Native Windows acceptance is pending / нативная приёмка Windows ещё не выполнена.


macOS Intel classic JIT memory policy, frame chain and host preparation / память JIT, цепочка кадров и подготовка host: [JIT_MACOS_X64.md](JIT_MACOS_X64.md). Apple Silicon/MAP_JIT and native macOS acceptance remain open / Apple Silicon/MAP_JIT и нативная приёмка остаются открытыми.
## Desktop host and authored stage

[`fvm_swf_stage_bounds`](../include/flashvm/swf.h#L38) reads authored signed-twip geometry;
[`fvm_movieclip_render_view`](../include/flashvm/movieclip.h#L59) projects rendering without modifying script poses.
Full parameters, ownership, errors and restrictions:
[EN](DESKTOP_FRONTEND.md#new-public-vm-api) / [RU](DESKTOP_FRONTEND.ru.md#новые-публичные-функции-vm).
## Incremental StartSound mixing

[`fvm_swf_sound_mixer_create`](../include/flashvm/swf_sound_mixer.h#L43), [`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L48),
[`fvm_swf_sound_mixer_read`](../include/flashvm/swf_sound_mixer.h#L53), [`fvm_swf_sound_mixer_info`](../include/flashvm/swf_sound_mixer.h#L57),
[`fvm_swf_sound_mixer_stop_all`](../include/flashvm/swf_sound_mixer.h#L59), [`fvm_swf_sound_mixer_close`](../include/flashvm/swf_sound_mixer.h#L61):
[full individual contracts](SWF_SOUND_MIXER.md) /
[русское руководство](SWF_SOUND_MIXER.ru.md),
[annotated C89 tutorial](../examples/multimedia/swf_event_mixer/README.md).
## MovieClip root sound notifications

[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L21) and [`FVM_MovieClipSoundCallback`](../include/flashvm/movieclip.h#L18) connect actual
root StartSound entry to host mixing, before DoAction. [EN contracts](MOVIECLIP_SOUND.md) /
[RU](MOVIECLIP_SOUND.ru.md). Sprite sound and automatic device-clock scheduling remain unsupported.
## Explicit mixer rate/channel conversion

[`fvm_swf_sound_mixer_create_configured`](../include/flashvm/swf_sound_mixer.h#L37), [`fvm_swf_sound_mixer_get_config`](../include/flashvm/swf_sound_mixer.h#L40),
[`FVM_SWFSoundMixerConfig`](../include/flashvm/swf_sound_mixer.h#L26), [`FVM_SWF_SOUND_MIXER_CONVERT`](../include/flashvm/swf_sound_mixer.h#L21):
[individual EN contracts](SWF_SOUND_CONVERSION.md) / [RU](SWF_SOUND_CONVERSION.ru.md).
Legacy strict creation and info ABI remain unchanged; conversion is opt-in.

## PCM output queue and consumed-frame clock

[`FVM_AudioQueueBackend`](../include/flashvm/swf_sound_output.h#L16), [`FVM_SWFSoundOutputInfo`](../include/flashvm/swf_sound_output.h#L33), [`fvm_swf_sound_output_create`](../include/flashvm/swf_sound_output.h#L48),
[`fvm_swf_sound_output_pump`](../include/flashvm/swf_sound_output.h#L52), [`fvm_swf_sound_output_pause`](../include/flashvm/swf_sound_output.h#L54), [`fvm_swf_sound_output_info`](../include/flashvm/swf_sound_output.h#L56),
[`fvm_swf_sound_output_stop`](../include/flashvm/swf_sound_output.h#L58), [`fvm_swf_sound_output_close`](../include/flashvm/swf_sound_output.h#L61), [`fvm_audio_sdl_queue_open`](../include/flashvm/swf_sound_output.h#L68)
and [`fvm_audio_sdl_queue_close`](../include/flashvm/swf_sound_output.h#L83): [individual EN contracts](SWF_SOUND_OUTPUT.md) /
[RU](SWF_SOUND_OUTPUT.ru.md), [C89 tutorial](../examples/multimedia/swf_audio_queue/README.md).
Host-owned queue integration and software FIFO clock; automatic desktop A/V
synchronization and exact hardware position remain open.

## Player test packages and host extensions

[Linux/Windows/Android build and HostInput contracts](PLAYER_TEST_BUILDS.md) /
[RU](PLAYER_TEST_BUILDS.ru.md), [two runnable SWF demos](../examples/player/animation_audio/README.md).
This is host integration of the existing C ABI; HostInput is not the complete
standard Flash Key/Mouse API. Native foreign-platform acceptance remains open.

## Prepared output stop registration and device geometry

- [`fvm_audio_stop_binding_create`](../include/flashvm/audio_stop.h#L14): context-owned stop registration; callbacks, ownership, errors and reentry in [AUDIO_STOP_BINDINGS.md](AUDIO_STOP_BINDINGS.md).
- [`fvm_audio_stop_binding_close`](../include/flashvm/audio_stop.h#L17): detach without invoking stop; same [contract](AUDIO_STOP_BINDINGS.md).
- [`fvm_audio_sdl_queue_info`](../include/flashvm/swf_sound_output.h#L81): copy immutable obtained rate/channels/device quantum; [SWF_SOUND_OUTPUT.md](SWF_SOUND_OUTPUT.md#obtained-device-geometry-v0583).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_active` | function | [include/flashvm/multimedia.h:90](../include/flashvm/multimedia.h#L90) |
| `fvm_audio_channel_close` | function | [include/flashvm/multimedia.h:88](../include/flashvm/multimedia.h#L88) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:82](../include/flashvm/multimedia.h#L82) |
| `fvm_audio_channel_stop` | function | [include/flashvm/multimedia.h:86](../include/flashvm/multimedia.h#L86) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:84](../include/flashvm/multimedia.h#L84) |
| `fvm_audio_linux_alsa_close` | function | [include/flashvm/multimedia.h:79](../include/flashvm/multimedia.h#L79) |
| `fvm_audio_linux_alsa_open` | function | [include/flashvm/multimedia.h:77](../include/flashvm/multimedia.h#L77) |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:131](../include/flashvm/multimedia.h#L131) |
| `fvm_audio_sdl_queue_close` | function | [include/flashvm/swf_sound_output.h:83](../include/flashvm/swf_sound_output.h#L83) |
| `fvm_audio_sdl_queue_info` | function | [include/flashvm/swf_sound_output.h:81](../include/flashvm/swf_sound_output.h#L81) |
| `fvm_audio_sdl_queue_open` | function | [include/flashvm/swf_sound_output.h:68](../include/flashvm/swf_sound_output.h#L68) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:92](../include/flashvm/multimedia.h#L92) |
| `fvm_audio_stop_binding_close` | function | [include/flashvm/audio_stop.h:17](../include/flashvm/audio_stop.h#L17) |
| `fvm_audio_stop_binding_create` | function | [include/flashvm/audio_stop.h:14](../include/flashvm/audio_stop.h#L14) |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:16](../include/flashvm/swf_sound_output.h#L16) |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `FVM_MEDIA_AUDIO_MIX_MAX_INPUTS` | macro / define | [include/flashvm/multimedia.h:19](../include/flashvm/multimedia.h#L19) |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](../include/flashvm/meta_call.h#L20) |
| `fvm_meta_register` | function | [include/flashvm/metamethod.h:20](../include/flashvm/metamethod.h#L20) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:59](../include/flashvm/movieclip.h#L59) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:21](../include/flashvm/movieclip.h#L21) |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:33](../include/flashvm/movieclip.h#L33) |
| `FVM_MovieClipSoundCallback` | callback typedef | [include/flashvm/movieclip.h:18](../include/flashvm/movieclip.h#L18) |
| `FVM_Object` | struct | [src/core/runtime.hpp:120](../src/core/runtime.hpp#L120) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
| `fvm_register_function` | function | [include/flashvm/value.h:101](../include/flashvm/value.h#L101) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../include/flashvm/flashvm.h#L66) |
| `fvm_stage_get_quality` | function | [include/flashvm/stage.h:19](../include/flashvm/stage.h#L19) |
| `fvm_stage_set_quality` | function | [include/flashvm/stage.h:22](../include/flashvm/stage.h#L22) |
| `fvm_stage_toggle_quality` | function | [include/flashvm/stage.h:25](../include/flashvm/stage.h#L25) |
| `fvm_swf_sound_at` | function | [include/flashvm/swf_sound.h:88](../include/flashvm/swf_sound.h#L88) |
| `fvm_swf_sound_count` | function | [include/flashvm/swf_sound.h:86](../include/flashvm/swf_sound.h#L86) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:58](../include/flashvm/swf_sound.h#L58) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:56](../include/flashvm/swf_sound.h#L56) |
| `fvm_swf_sound_event_envelope` | function | [include/flashvm/swf_sound.h:60](../include/flashvm/swf_sound.h#L60) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:97](../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:90](../include/flashvm/swf_sound.h#L90) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:61](../include/flashvm/swf_sound_mixer.h#L61) |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:21](../include/flashvm/swf_sound_mixer.h#L21) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:43](../include/flashvm/swf_sound_mixer.h#L43) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:37](../include/flashvm/swf_sound_mixer.h#L37) |
| `fvm_swf_sound_mixer_get_config` | function | [include/flashvm/swf_sound_mixer.h:40](../include/flashvm/swf_sound_mixer.h#L40) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:57](../include/flashvm/swf_sound_mixer.h#L57) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:53](../include/flashvm/swf_sound_mixer.h#L53) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:59](../include/flashvm/swf_sound_mixer.h#L59) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:48](../include/flashvm/swf_sound_mixer.h#L48) |
| `fvm_swf_sound_output_close` | function | [include/flashvm/swf_sound_output.h:61](../include/flashvm/swf_sound_output.h#L61) |
| `fvm_swf_sound_output_create` | function | [include/flashvm/swf_sound_output.h:48](../include/flashvm/swf_sound_output.h#L48) |
| `fvm_swf_sound_output_info` | function | [include/flashvm/swf_sound_output.h:56](../include/flashvm/swf_sound_output.h#L56) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:54](../include/flashvm/swf_sound_output.h#L54) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:52](../include/flashvm/swf_sound_output.h#L52) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:58](../include/flashvm/swf_sound_output.h#L58) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:93](../include/flashvm/swf_sound.h#L93) |
| `fvm_swf_sound_submit` | function | [include/flashvm/swf_sound.h:100](../include/flashvm/swf_sound.h#L100) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:84](../include/flashvm/swf_sound.h#L84) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:80](../include/flashvm/swf_sound.h#L80) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:78](../include/flashvm/swf_sound.h#L78) |
| `fvm_swf_sound_voice_read` | function | [include/flashvm/swf_sound.h:82](../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_stage_bounds` | function | [include/flashvm/swf.h:38](../include/flashvm/swf.h#L38) |
| `FVM_SWFSoundEnvelope` | struct | [include/flashvm/swf_sound.h:48](../include/flashvm/swf_sound.h#L48) |
| `FVM_SWFSoundEvent` | struct | [include/flashvm/swf_sound.h:36](../include/flashvm/swf_sound.h#L36) |
| `FVM_SWFSoundInfo` | struct | [include/flashvm/swf_sound.h:13](../include/flashvm/swf_sound.h#L13) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:26](../include/flashvm/swf_sound_mixer.h#L26) |
| `FVM_SWFSoundOutputInfo` | struct | [include/flashvm/swf_sound_output.h:33](../include/flashvm/swf_sound_output.h#L33) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:9](../src/swf/sound_voice.cpp#L9) |
| `FVM_SWFSoundVoiceInfo` | struct | [include/flashvm/swf_sound.h:63](../include/flashvm/swf_sound.h#L63) |
| `fvm_table_set_metatable` | function | [include/flashvm/table.h:24](../include/flashvm/table.h#L24) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
