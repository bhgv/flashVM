# FlashVM C89/C++98 API index
- [Resource frames](PLAYER_RESOURCE_FRAMES.md) / [RU](PLAYER_RESOURCE_FRAMES.ru.md): bounded copied RGBA, session-owned leases and chunked reads.
- [Player resource state](PLAYER_RESOURCE_STATE.md) / [RU](PLAYER_RESOURCE_STATE.ru.md): owner snapshots, root epochs and independent READY/TICK/policy observations.
- [Resource 9P2000](RESOURCE_9P.md) / [RU](RESOURCE_9P.ru.md): `r9p.h`, bounded device-message adapter with host-owned transport.
- [Resource files](RESOURCE_FILES.md) / [RU](RESOURCE_FILES.ru.md): `rfs.h`, native open/read/write/close over dispatcher resources.

- [Hybrid heartbeat](HYBRID_HEARTBEAT.md) / [RU](HYBRID_HEARTBEAT.ru.md): `rheartbeat.h`, separate UDP/TCP policy and checked hints.

- [Resource debugger](RESOURCE_DEBUGGER.md) / [RU](RESOURCE_DEBUGGER.ru.md): `rdebug.h`, owner commands, copied frames and multiple observers.

- [Resource observers](RESOURCE_EVENTS.md) / [RU](RESOURCE_EVENTS.ru.md): `revents.h`, independent event queues, discovery and disconnect.

- `include/flashvm/gc.h`: collection, mode, pinning, introspection.
- `include/flashvm/value.h`: primitive and object values.
- `include/flashvm/table.h`: indexed elements and metatables.
- `include/flashvm/channel.h`: raw channel transport.
- `include/flashvm/transfer.h`: graph send/receive across queue/file/TCP channels.
- `include/flashvm/property.h`: property flags, deletion and enumerable-name snapshots.
- `include/flashvm/function.h`: first-class native/AVM1 functions, invoke/construct.
- `include/flashvm/avm1.h`: actions, rooted trace/URL sinks and borrowed active-target introspection.
- [stage.h](../include/flashvm/stage.h): host background/quality plus [fvm_stage_request_update](../include/flashvm/stage.h#L52), [fvm_stage_get_update](../include/flashvm/stage.h#L57) and [fvm_stage_take_update](../include/flashvm/stage.h#L63) ([EN](UPDATE_AFTER_EVENT.md) / [RU](UPDATE_AFTER_EVENT.ru.md)).
- [clip_actions.h](../include/flashvm/clip_actions.h): [fvm_movieclip_dispatch_clip_actions](../include/flashvm/clip_actions.h#L58), [`fvm_movieclip_dispatch_clip_input`](../include/flashvm/clip_actions.h#L76) for retained scene records, [`fvm_movieclip_dispatch_input`](../include/flashvm/clip_actions.h#L93) for records plus methods ([EN](CLIP_INPUT_METHODS.md) / [RU](CLIP_INPUT_METHODS.ru.md)), and individual FVM_CLIP_EVENT_* selectors ([EN](CLIP_ACTIONS.md) / [RU](CLIP_ACTIONS.ru.md)).
- [movieclip.h](../include/flashvm/movieclip.h): host-authored clip frames, vector rendering and [fvm_movieclip_advance](../include/flashvm/movieclip.h#L160) ([EN ticks](MOVIECLIP_TICKS.md) / [RU](MOVIECLIP_TICKS.ru.md)).
- `include/flashvm/standard.h`: standard installation and primitive conversion.
- [timers.h](../include/flashvm/timers.h): [fvm_timers_poll](../include/flashvm/timers.h#L22), explicit owning-thread scheduled callback delivery ([EN](TIMERS.md) / [RU](TIMERS.ru.md)).
- `include/flashvm/capability.h`: TCP resource authority and imports.

All public headers must remain valid C89 and usable from C++98. No STL types cross the C ABI.

Object graph coverage and limitations: [EN](STANDARD_OBJECTS.md), [RU](STANDARD_OBJECTS.ru.md).

`value.h`: ABI 3 counted UTF-8/WTF-8 and UTF-16 creation/query/copy helpers.

[`fvm_movieclip_swf_document_info`](../include/flashvm/movieclip.h#L41) — [SWF document metadata / метаданные SWF](SWF_DOCUMENT_METADATA.md), `include/flashvm/movieclip.h`.

## Runtime and context

Initialize [`FVM_Config`](../include/flashvm/flashvm.h#L55) with [`fvm_config_init`](../include/flashvm/flashvm.h#L70), create the runtime with
[`fvm_runtime_create`](../include/flashvm/flashvm.h#L74), then create its context with [`fvm_context_create`](../include/flashvm/flashvm.h#L82).
Check each returned [`FVM_Result`](../include/flashvm/flashvm.h#L42) before using an output handle. Destroy the
context with [`fvm_context_destroy`](../include/flashvm/flashvm.h#L86) before [`fvm_runtime_destroy`](../include/flashvm/flashvm.h#L78). Each context
owns its managed object graph; do not access one context concurrently from
multiple threads. Worker examples create independent contexts and exchange
values through the documented transport API.

Сначала инициализируйте конфигурацию, затем создайте runtime и context.
Проверяйте результаты до использования выходных handles; уничтожайте context
раньше runtime. Контекст владеет своим графом объектов и используется его
VM-потоком. Точные декларации: [flashvm.h](../include/flashvm/flashvm.h).

## Values and borrowed views

[`FVM_Value`](../include/flashvm/value.h#L26) carries a type tag and its corresponding payload. Use the
`fvm_value_*` helpers instead of leaving cells uninitialized. A borrowed string
or object view does not transfer ownership. Keep backing bytes valid for the
operation that borrows them; use counted string APIs when embedded NUL matters.
Managed objects retained by the host across GC safe points need explicit roots;
see [GC API](GARBAGE_COLLECTION.md#api). Cross-context object transfer uses
serialization/resource interfaces, not copying `FVM_Object *` pointers.

Ячейка [FVM_Value](../include/flashvm/value.h#L26) хранит тег и значение. Заимствованное представление не передаёт
владение; время жизни конкретного результата определяется вызванной функцией.
Для строк со встроенным NUL используйте API с длиной, для объектов через точки
GC — явные корни. Декларации и границы lifetime: [value.h](../include/flashvm/value.h).

## Native calls and metatables

[`fvm_register_function`](../include/flashvm/value.h#L153) installs a named native callback; [`fvm_call`](../include/flashvm/value.h#L204) invokes a
named function and [`fvm_object_call`](../include/flashvm/value.h#L184) invokes a named method with a receiver.
Check the current declarations in [value.h](../include/flashvm/value.h), and
[AVM1 function contracts](AVM1_FUNCTIONS.md) for script invocation. Registration
userdata belongs to the host; a callback must respect the owning VM thread.

[`fvm_table_set_metatable`](../include/flashvm/table.h#L30) attaches a table; [`fvm_meta_register`](../include/flashvm/metamethod.h#L22) registers a
metamethod. The explicit [`fvm_meta_call`](../include/flashvm/meta_call.h#L22) API invokes native __call with owner,
defining metatable and a fresh indexed argument table (user argument zero is at
index zero). This operation has its own callback convention; it does not imply
implicit AVM1 closure dispatch. See [meta_call.h](../include/flashvm/meta_call.h),
[metamethod.h](../include/flashvm/metamethod.h) and the
[annotated meta-call description](../examples/extending/meta_call/README.ru.md).

Именованные native-функции, методы с receiver и явный meta-call — разные
соглашения вызова. Выбирайте API по примеру и не переносите правила аргументов
одного соглашения на другое. Stack callbacks описаны отдельно в [STACK_API.md](STACK_API.md).
Documentation maintenance / Ведение документации: [rules and workflow](DOCUMENTATION_GUIDELINES.md).

Stage policy: [`fvm_stage_get_quality`](../include/flashvm/stage.h#L21), [`fvm_stage_set_quality`](../include/flashvm/stage.h#L26), [`fvm_stage_toggle_quality`](../include/flashvm/stage.h#L31)
— [STAGE_QUALITY.md](STAGE_QUALITY.md), including parameters, statuses, lifetime,
ToggleQuality transitions and renderer limits.
## Multimedia and devices / Мультимедиа и устройства

[Bridge API and detailed contracts](MULTIMEDIA_BRIDGES.md); [public header](../include/flashvm/multimedia.h).
Decoder/capture sessions, synchronous PCM/RGBA output, native input and future sensor providers.
Native/interpreter StopSounds equivalence: [JIT_SOUND.md](JIT_SOUND.md).

Root streaming-sound segments, frame blocks and bounded PCM reads:
[SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md), [MP3 prefix/encoded snapshots](SWF_STREAM_MP3.md), [swf_stream_sound.h](../include/flashvm/swf_stream_sound.h).

Context audio registration and script stopping: [AUDIO_CHANNELS.md](AUDIO_CHANNELS.md).
[`fvm_audio_channel_create`](../include/flashvm/multimedia.h#L100), [`fvm_audio_channel_write`](../include/flashvm/multimedia.h#L104), [`fvm_audio_channel_stop`](../include/flashvm/multimedia.h#L108),
[`fvm_audio_channel_close`](../include/flashvm/multimedia.h#L112), [`fvm_audio_channel_active`](../include/flashvm/multimedia.h#L116) and [`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L120)
provide copied output tables, explicit ownership and shared StopSounds semantics.

Native Linux output: [`fvm_audio_linux_alsa_open`](../include/flashvm/multimedia.h#L91), [`fvm_audio_linux_alsa_close`](../include/flashvm/multimedia.h#L95) —
[static ALSA provider contracts](ALSA_AUDIO_OUTPUT.md).

Loaded SWF audio resource/StartSound inspection and explicit PCM output: [`FVM_SWFSoundInfo`](../include/flashvm/swf_sound.h#L16),
[`fvm_swf_sound_count`](../include/flashvm/swf_sound.h#L166), [`fvm_swf_sound_at`](../include/flashvm/swf_sound.h#L170), [`fvm_swf_sound_info`](../include/flashvm/swf_sound.h#L174),
[`fvm_swf_sound_read_pcm`](../include/flashvm/swf_sound.h#L190), [`fvm_swf_sound_submit`](../include/flashvm/swf_sound.h#L201) —
[SWF_SOUND_RESOURCES.md](SWF_SOUND_RESOURCES.md), [public header](../include/flashvm/swf_sound.h).
Copied timeline metadata: [`FVM_SWFSoundEvent`](../include/flashvm/swf_sound.h#L42), [`FVM_SWFSoundEnvelope`](../include/flashvm/swf_sound.h#L56),
[`fvm_swf_sound_event_count`](../include/flashvm/swf_sound.h#L70), [`fvm_swf_sound_event_at`](../include/flashvm/swf_sound.h#L74), [`fvm_swf_sound_event_envelope`](../include/flashvm/swf_sound.h#L78) —
[StartSound contracts](SWF_SOUND_RESOURCES.md#root-startsound-records--события-startsound).
These inspect controls without automatically scheduling playback.
[`fvm_swf_sound_event_submit`](../include/flashvm/swf_sound.h#L196) applies one bounded styled voice to a dedicated
channel: [playback contract](SWF_SOUND_PLAYBACK.md).

Prepared independent sequential voices: [`FVM_SWFSoundVoice`](../src/swf/sound_voice.cpp#L12), [`FVM_SWFSoundVoiceInfo`](../include/flashvm/swf_sound.h#L83),
[`fvm_swf_sound_voice_open`](../include/flashvm/swf_sound.h#L100), [`fvm_swf_sound_voice_read`](../include/flashvm/swf_sound.h#L158), [`fvm_swf_sound_voice_info`](../include/flashvm/swf_sound.h#L142),
[`fvm_swf_sound_voice_close`](../include/flashvm/swf_sound.h#L162) — [incremental voice contracts](SWF_SOUND_VOICES.md).
Source PCM is retained once; snapshot reads allocate nothing and outlive the VM.

Portable bounded S16 summing: [`fvm_audio_mix_s16`](../include/flashvm/multimedia.h#L179), [`FVM_MEDIA_AUDIO_MIX_MAX_INPUTS`](../include/flashvm/multimedia.h#L19) —
[AUDIO_MIXING.md](AUDIO_MIXING.md). Caller-aligned blocks, final-only saturation,
short-tail silence and no allocation/implicit device or clock.

## Speech / Речевые службы

[Detailed TTS/STT API and per-OS preparation](SPEECH_BRIDGES.md); [public speech.h](../include/flashvm/speech.h).
Separate explicit voice and microphone sessions, replaceable native providers, retained UTF-8 transcript query/copy.


Prepared PCM stream snapshots / Независимые индексированные снимки PCM: [SWF_STREAM_SOUND.md](SWF_STREAM_SOUND.md#prepared-immutable-pcm-index--подготовленный-неизменяемый-индекс-pcm). Six C89 functions copy/index one validated root PCM segment; indexed reads survive source VM destruction without allocation. Preparation is explicitly budgeted; compressed codecs and automatic MovieClip audio clocks remain unsupported.


Windows x64 backend selection, W^X and OS unwind-table ownership / выбор Windows x64 backend, W^X и время жизни unwind: [JIT_WINDOWS_X64.md](JIT_WINDOWS_X64.md). Native Windows acceptance is pending / нативная приёмка Windows ещё не выполнена.


macOS Intel classic JIT memory policy, frame chain and host preparation / память JIT, цепочка кадров и подготовка host: [JIT_MACOS_X64.md](JIT_MACOS_X64.md). Apple Silicon/MAP_JIT and native macOS acceptance remain open / Apple Silicon/MAP_JIT и нативная приёмка остаются открытыми.
## Desktop host and authored stage

[`fvm_swf_stage_bounds`](../include/flashvm/swf.h#L45) reads authored signed-twip geometry;
[`fvm_movieclip_render_view`](../include/flashvm/movieclip.h#L179) projects rendering without modifying script poses.
Full parameters, ownership, errors and restrictions:
[EN](DESKTOP_FRONTEND.md#new-public-vm-api) / [RU](DESKTOP_FRONTEND.ru.md#новые-публичные-функции-vm).
## Incremental StartSound mixing

[`fvm_swf_sound_mixer_create`](../include/flashvm/swf_sound_mixer.h#L88), [`fvm_swf_sound_mixer_submit`](../include/flashvm/swf_sound_mixer.h#L95),
[`fvm_swf_sound_mixer_read`](../include/flashvm/swf_sound_mixer.h#L157), [`fvm_swf_sound_mixer_info`](../include/flashvm/swf_sound_mixer.h#L176),
[`fvm_swf_sound_mixer_stop_all`](../include/flashvm/swf_sound_mixer.h#L180), [`fvm_swf_sound_mixer_close`](../include/flashvm/swf_sound_mixer.h#L184):
[full individual contracts](SWF_SOUND_MIXER.md) /
[русское руководство](SWF_SOUND_MIXER.ru.md),
[annotated C89 tutorial](../examples/multimedia/swf_event_mixer/README.md).
## MovieClip root sound notifications

[`fvm_movieclip_set_sound_callback`](../include/flashvm/movieclip.h#L25) and [`FVM_MovieClipSoundCallback`](../include/flashvm/movieclip.h#L20) connect actual
root StartSound entry to host mixing, before DoAction. [EN contracts](MOVIECLIP_SOUND.md) /
[RU](MOVIECLIP_SOUND.ru.md). Sprite sound and automatic device-clock scheduling remain unsupported.
## Explicit mixer rate/channel conversion

[`fvm_swf_sound_mixer_create_configured`](../include/flashvm/swf_sound_mixer.h#L69), [`fvm_swf_sound_mixer_get_config`](../include/flashvm/swf_sound_mixer.h#L83),
[`FVM_SWFSoundMixerConfig`](../include/flashvm/swf_sound_mixer.h#L33), [`FVM_SWF_SOUND_MIXER_CONVERT`](../include/flashvm/swf_sound_mixer.h#L24):
[individual EN contracts](SWF_SOUND_CONVERSION.md) / [RU](SWF_SOUND_CONVERSION.ru.md).
Legacy strict creation and info ABI remain unchanged; conversion is opt-in.

## PCM output queue and consumed-frame clock

[`FVM_AudioQueueBackend`](../include/flashvm/swf_sound_output.h#L18), [`FVM_SWFSoundOutputInfo`](../include/flashvm/swf_sound_output.h#L38), [`fvm_swf_sound_output_create`](../include/flashvm/swf_sound_output.h#L56),
[`fvm_swf_sound_output_pump`](../include/flashvm/swf_sound_output.h#L148), [`fvm_swf_sound_output_pause`](../include/flashvm/swf_sound_output.h#L174), [`fvm_swf_sound_output_info`](../include/flashvm/swf_sound_output.h#L178),
[`fvm_swf_sound_output_stop`](../include/flashvm/swf_sound_output.h#L184), [`fvm_swf_sound_output_close`](../include/flashvm/swf_sound_output.h#L201), [`fvm_audio_sdl_queue_open`](../include/flashvm/swf_sound_output.h#L210)
and [`fvm_audio_sdl_queue_close`](../include/flashvm/swf_sound_output.h#L232): [individual EN contracts](SWF_SOUND_OUTPUT.md) /
[RU](SWF_SOUND_OUTPUT.ru.md), [C89 tutorial](../examples/multimedia/swf_audio_queue/README.md).
Host-owned queue integration and software FIFO clock; automatic desktop A/V
synchronization and exact hardware position remain open.

## Player test packages and host extensions

[Linux/Windows/Android build and HostInput contracts](PLAYER_TEST_BUILDS.md) /
[RU](PLAYER_TEST_BUILDS.ru.md), [two runnable SWF demos](../examples/player/animation_audio/README.md).
This is host integration of the existing C ABI; HostInput is not the complete
standard Flash Key/Mouse API. Native foreign-platform acceptance remains open.

## Prepared output stop registration and device geometry

- [`fvm_audio_stop_binding_create`](../include/flashvm/audio_stop.h#L19): context-owned stop registration; callbacks, ownership, errors and reentry in [AUDIO_STOP_BINDINGS.md](AUDIO_STOP_BINDINGS.md).
- [`fvm_audio_stop_binding_close`](../include/flashvm/audio_stop.h#L24): detach without invoking stop; same [contract](AUDIO_STOP_BINDINGS.md).
- [`fvm_audio_sdl_queue_info`](../include/flashvm/swf_sound_output.h#L228): copy immutable obtained rate/channels/device quantum; [SWF_SOUND_OUTPUT.md](SWF_SOUND_OUTPUT.md#obtained-device-geometry-v0583).

## Optional player release (v0.587)

- [`FVM_SWF_SOUND_MIXER_DECLICK`](../include/flashvm/swf_sound_mixer.h#L26): stop-by-ID voice envelope; [contract](PLAYER_AUDIO_RELEASE.md#stop-by-resource-voices).
- [`fvm_swf_sound_output_set_release`](../include/flashvm/swf_sound_output.h#L188): transactional optional history configuration before submission; [arguments/status/ownership](PLAYER_AUDIO_RELEASE.md#fvm_swf_sound_output_set_release).
- [`fvm_swf_sound_output_stop_release`](../include/flashvm/swf_sound_output.h#L196): running queue replacement and bounded tail; [errors/retry/limits](PLAYER_AUDIO_RELEASE.md#fvm_swf_sound_output_stop_release).


[Keyboard snapshot and Key polling / клавиатурный снимок и опрос Key](KEYBOARD.md) ([RU](KEYBOARD.ru.md)): [`fvm_input_set_keyboard`](../include/flashvm/keyboard.h#L73), [`fvm_input_get_keyboard`](../include/flashvm/keyboard.h#L77), [`fvm_input_reset_keyboard`](../include/flashvm/keyboard.h#L81), [`fvm_standard_keyboard_install`](../include/flashvm/keyboard.h#L127).


[Keyboard listeners / слушатели клавиатуры](KEYBOARD_LISTENERS.md) ([RU](KEYBOARD_LISTENERS.ru.md)): [`fvm_input_add_key_listener`](../include/flashvm/keyboard.h#L88), [`fvm_input_remove_key_listener`](../include/flashvm/keyboard.h#L92), [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L108).


[Explicit host keyboard focus / Явный выбор клипа](KEYBOARD_FOCUS.md) ([RU](KEYBOARD_FOCUS.ru.md)): [`fvm_input_set_keyboard_focus`](../include/flashvm/keyboard.h#L97), [`fvm_input_get_keyboard_focus`](../include/flashvm/keyboard.h#L102).

[Selection MovieClip subset / Скриптовый выбор клипа](SELECTION.md) ([RU](SELECTION.ru.md)): [`fvm_standard_selection_install`](../include/flashvm/keyboard.h#L123).


Observer-only scene composition v0.627: [`fvm_input_dispatch_key_listeners`](../include/flashvm/keyboard.h#L117) publishes
the validated snapshot and notifies registered Key listeners, without an implicit
focused-clip call. The original [`fvm_input_dispatch_keyboard`](../include/flashvm/keyboard.h#L108) is unchanged. Use the
new route before whole-scene methods to avoid duplicate focus delivery. Explicit
listener registration remains independent. [EN](CLIP_INPUT_METHODS.md) /
[RU](CLIP_INPUT_METHODS.ru.md).


KeyPress v0.628: [`fvm_input_clip_key_code`](../include/flashvm/keyboard.h#L68) validates a copied transition and maps
special/printable selectors without allocation or state mutation.
[`fvm_movieclip_dispatch_key_press`](../include/flashvm/clip_actions.h#L106) separately broadcasts exact retained selectors.
[EN contract](CLIP_KEYPRESS.md) / [RU](CLIP_KEYPRESS.ru.md).
[Atomic native MovieClip unloading and attachment query](MOVIE_UNLOAD.md) / [RU](MOVIE_UNLOAD.ru.md).
[Standalone image replacement / Изображения в клипе](IMAGE_LOADING.md).

[Named MovieClip dimensions / Именованные размеры](MOVIECLIP_NAMED_PROPERTIES.md) / [RU](MOVIECLIP_NAMED_PROPERTIES.ru.md).

[Committed content metadata / Сведения о содержимом](MOVIECLIP_CONTENT_INFO.md) / [RU](MOVIECLIP_CONTENT_INFO.ru.md).

[Owner playback commands / Управление воспроизведением](PLAYBACK_CONTROL.md) / [RU](PLAYBACK_CONTROL.ru.md).
[Thread-safe playback mailbox / Межпоточный ящик команд](PLAYBACK_MAILBOX.md) / [RU](PLAYBACK_MAILBOX.ru.md).
[RGBA frame observation / Наблюдение RGBA-кадров](FRAME_CAPTURE.md) / [RU](FRAME_CAPTURE.ru.md).
Windowless logical-stage adapter / Адаптер без окна: [EN](OFFSCREEN_PLAYER.md) / [RU](OFFSCREEN_PLAYER.ru.md).
Binary S-record codec / Двоичный codec: [EN](SRECORD.md) / [RU](SRECORD.ru.md).
Accepted resource/filesystem design / Принятый план ФС ресурсов: [EN](RESOURCE_NAMESPACE.md) / [RU](RESOURCE_NAMESPACE.ru.md).
Resource dispatcher / Диспетчер: [EN](RESOURCE_DISPATCH.md) / [RU](RESOURCE_DISPATCH.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_active` | function | [include/flashvm/multimedia.h:116](../include/flashvm/multimedia.h#L116) |
| `fvm_audio_channel_close` | function | [include/flashvm/multimedia.h:112](../include/flashvm/multimedia.h#L112) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:100](../include/flashvm/multimedia.h#L100) |
| `fvm_audio_channel_stop` | function | [include/flashvm/multimedia.h:108](../include/flashvm/multimedia.h#L108) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:104](../include/flashvm/multimedia.h#L104) |
| `fvm_audio_linux_alsa_close` | function | [include/flashvm/multimedia.h:95](../include/flashvm/multimedia.h#L95) |
| `fvm_audio_linux_alsa_open` | function | [include/flashvm/multimedia.h:91](../include/flashvm/multimedia.h#L91) |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:179](../include/flashvm/multimedia.h#L179) |
| `fvm_audio_sdl_queue_close` | function | [include/flashvm/swf_sound_output.h:232](../include/flashvm/swf_sound_output.h#L232) |
| `fvm_audio_sdl_queue_info` | function | [include/flashvm/swf_sound_output.h:228](../include/flashvm/swf_sound_output.h#L228) |
| `fvm_audio_sdl_queue_open` | function | [include/flashvm/swf_sound_output.h:210](../include/flashvm/swf_sound_output.h#L210) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:120](../include/flashvm/multimedia.h#L120) |
| `fvm_audio_stop_binding_close` | function | [include/flashvm/audio_stop.h:24](../include/flashvm/audio_stop.h#L24) |
| `fvm_audio_stop_binding_create` | function | [include/flashvm/audio_stop.h:19](../include/flashvm/audio_stop.h#L19) |
| `FVM_AudioQueueBackend` | struct | [include/flashvm/swf_sound_output.h:18](../include/flashvm/swf_sound_output.h#L18) |
| `fvm_call` | function | [include/flashvm/value.h:204](../include/flashvm/value.h#L204) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:55](../include/flashvm/flashvm.h#L55) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:82](../include/flashvm/flashvm.h#L82) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:86](../include/flashvm/flashvm.h#L86) |
| `fvm_input_add_key_listener` | function | [include/flashvm/keyboard.h:88](../include/flashvm/keyboard.h#L88) |
| `fvm_input_clip_key_code` | function | [include/flashvm/keyboard.h:68](../include/flashvm/keyboard.h#L68) |
| `fvm_input_dispatch_key_listeners` | function | [include/flashvm/keyboard.h:117](../include/flashvm/keyboard.h#L117) |
| `fvm_input_dispatch_keyboard` | function | [include/flashvm/keyboard.h:108](../include/flashvm/keyboard.h#L108) |
| `fvm_input_get_keyboard` | function | [include/flashvm/keyboard.h:77](../include/flashvm/keyboard.h#L77) |
| `fvm_input_get_keyboard_focus` | function | [include/flashvm/keyboard.h:102](../include/flashvm/keyboard.h#L102) |
| `fvm_input_remove_key_listener` | function | [include/flashvm/keyboard.h:92](../include/flashvm/keyboard.h#L92) |
| `fvm_input_reset_keyboard` | function | [include/flashvm/keyboard.h:81](../include/flashvm/keyboard.h#L81) |
| `fvm_input_set_keyboard` | function | [include/flashvm/keyboard.h:73](../include/flashvm/keyboard.h#L73) |
| `fvm_input_set_keyboard_focus` | function | [include/flashvm/keyboard.h:97](../include/flashvm/keyboard.h#L97) |
| `FVM_MEDIA_AUDIO_MIX_MAX_INPUTS` | macro / define | [include/flashvm/multimedia.h:19](../include/flashvm/multimedia.h#L19) |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:22](../include/flashvm/meta_call.h#L22) |
| `fvm_meta_register` | function | [include/flashvm/metamethod.h:22](../include/flashvm/metamethod.h#L22) |
| `fvm_movieclip_advance` | function | [include/flashvm/movieclip.h:160](../include/flashvm/movieclip.h#L160) |
| `fvm_movieclip_dispatch_clip_actions` | function | [include/flashvm/clip_actions.h:58](../include/flashvm/clip_actions.h#L58) |
| `fvm_movieclip_dispatch_clip_input` | function | [include/flashvm/clip_actions.h:76](../include/flashvm/clip_actions.h#L76) |
| `fvm_movieclip_dispatch_input` | function | [include/flashvm/clip_actions.h:93](../include/flashvm/clip_actions.h#L93) |
| `fvm_movieclip_dispatch_key_press` | function | [include/flashvm/clip_actions.h:106](../include/flashvm/clip_actions.h#L106) |
| `fvm_movieclip_render_view` | function | [include/flashvm/movieclip.h:179](../include/flashvm/movieclip.h#L179) |
| `fvm_movieclip_set_sound_callback` | function | [include/flashvm/movieclip.h:25](../include/flashvm/movieclip.h#L25) |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:41](../include/flashvm/movieclip.h#L41) |
| `FVM_MovieClipSoundCallback` | callback typedef | [include/flashvm/movieclip.h:20](../include/flashvm/movieclip.h#L20) |
| `FVM_Object` | struct | [src/core/runtime.hpp:133](../src/core/runtime.hpp#L133) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../include/flashvm/value.h#L184) |
| `fvm_register_function` | function | [include/flashvm/value.h:153](../include/flashvm/value.h#L153) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:74](../include/flashvm/flashvm.h#L74) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:78](../include/flashvm/flashvm.h#L78) |
| `fvm_stage_get_quality` | function | [include/flashvm/stage.h:21](../include/flashvm/stage.h#L21) |
| `fvm_stage_get_update` | function | [include/flashvm/stage.h:57](../include/flashvm/stage.h#L57) |
| `fvm_stage_request_update` | function | [include/flashvm/stage.h:52](../include/flashvm/stage.h#L52) |
| `fvm_stage_set_quality` | function | [include/flashvm/stage.h:26](../include/flashvm/stage.h#L26) |
| `fvm_stage_take_update` | function | [include/flashvm/stage.h:63](../include/flashvm/stage.h#L63) |
| `fvm_stage_toggle_quality` | function | [include/flashvm/stage.h:31](../include/flashvm/stage.h#L31) |
| `fvm_standard_keyboard_install` | function | [include/flashvm/keyboard.h:127](../include/flashvm/keyboard.h#L127) |
| `fvm_standard_selection_install` | function | [include/flashvm/keyboard.h:123](../include/flashvm/keyboard.h#L123) |
| `fvm_swf_sound_at` | function | [include/flashvm/swf_sound.h:170](../include/flashvm/swf_sound.h#L170) |
| `fvm_swf_sound_count` | function | [include/flashvm/swf_sound.h:166](../include/flashvm/swf_sound.h#L166) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:74](../include/flashvm/swf_sound.h#L74) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:70](../include/flashvm/swf_sound.h#L70) |
| `fvm_swf_sound_event_envelope` | function | [include/flashvm/swf_sound.h:78](../include/flashvm/swf_sound.h#L78) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:196](../include/flashvm/swf_sound.h#L196) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:174](../include/flashvm/swf_sound.h#L174) |
| `fvm_swf_sound_mixer_close` | function | [include/flashvm/swf_sound_mixer.h:184](../include/flashvm/swf_sound_mixer.h#L184) |
| `FVM_SWF_SOUND_MIXER_CONVERT` | macro / define | [include/flashvm/swf_sound_mixer.h:24](../include/flashvm/swf_sound_mixer.h#L24) |
| `fvm_swf_sound_mixer_create` | function | [include/flashvm/swf_sound_mixer.h:88](../include/flashvm/swf_sound_mixer.h#L88) |
| `fvm_swf_sound_mixer_create_configured` | function | [include/flashvm/swf_sound_mixer.h:69](../include/flashvm/swf_sound_mixer.h#L69) |
| `FVM_SWF_SOUND_MIXER_DECLICK` | macro / define | [include/flashvm/swf_sound_mixer.h:26](../include/flashvm/swf_sound_mixer.h#L26) |
| `fvm_swf_sound_mixer_get_config` | function | [include/flashvm/swf_sound_mixer.h:83](../include/flashvm/swf_sound_mixer.h#L83) |
| `fvm_swf_sound_mixer_info` | function | [include/flashvm/swf_sound_mixer.h:176](../include/flashvm/swf_sound_mixer.h#L176) |
| `fvm_swf_sound_mixer_read` | function | [include/flashvm/swf_sound_mixer.h:157](../include/flashvm/swf_sound_mixer.h#L157) |
| `fvm_swf_sound_mixer_stop_all` | function | [include/flashvm/swf_sound_mixer.h:180](../include/flashvm/swf_sound_mixer.h#L180) |
| `fvm_swf_sound_mixer_submit` | function | [include/flashvm/swf_sound_mixer.h:95](../include/flashvm/swf_sound_mixer.h#L95) |
| `fvm_swf_sound_output_close` | function | [include/flashvm/swf_sound_output.h:201](../include/flashvm/swf_sound_output.h#L201) |
| `fvm_swf_sound_output_create` | function | [include/flashvm/swf_sound_output.h:56](../include/flashvm/swf_sound_output.h#L56) |
| `fvm_swf_sound_output_info` | function | [include/flashvm/swf_sound_output.h:178](../include/flashvm/swf_sound_output.h#L178) |
| `fvm_swf_sound_output_pause` | function | [include/flashvm/swf_sound_output.h:174](../include/flashvm/swf_sound_output.h#L174) |
| `fvm_swf_sound_output_pump` | function | [include/flashvm/swf_sound_output.h:148](../include/flashvm/swf_sound_output.h#L148) |
| `fvm_swf_sound_output_set_release` | function | [include/flashvm/swf_sound_output.h:188](../include/flashvm/swf_sound_output.h#L188) |
| `fvm_swf_sound_output_stop` | function | [include/flashvm/swf_sound_output.h:184](../include/flashvm/swf_sound_output.h#L184) |
| `fvm_swf_sound_output_stop_release` | function | [include/flashvm/swf_sound_output.h:196](../include/flashvm/swf_sound_output.h#L196) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:190](../include/flashvm/swf_sound.h#L190) |
| `fvm_swf_sound_submit` | function | [include/flashvm/swf_sound.h:201](../include/flashvm/swf_sound.h#L201) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:162](../include/flashvm/swf_sound.h#L162) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:142](../include/flashvm/swf_sound.h#L142) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:100](../include/flashvm/swf_sound.h#L100) |
| `fvm_swf_sound_voice_read` | function | [include/flashvm/swf_sound.h:158](../include/flashvm/swf_sound.h#L158) |
| `fvm_swf_stage_bounds` | function | [include/flashvm/swf.h:45](../include/flashvm/swf.h#L45) |
| `FVM_SWFSoundEnvelope` | struct | [include/flashvm/swf_sound.h:56](../include/flashvm/swf_sound.h#L56) |
| `FVM_SWFSoundEvent` | struct | [include/flashvm/swf_sound.h:42](../include/flashvm/swf_sound.h#L42) |
| `FVM_SWFSoundInfo` | struct | [include/flashvm/swf_sound.h:16](../include/flashvm/swf_sound.h#L16) |
| `FVM_SWFSoundMixerConfig` | struct | [include/flashvm/swf_sound_mixer.h:33](../include/flashvm/swf_sound_mixer.h#L33) |
| `FVM_SWFSoundOutputInfo` | struct | [include/flashvm/swf_sound_output.h:38](../include/flashvm/swf_sound_output.h#L38) |
| `FVM_SWFSoundVoice` | struct | [src/swf/sound_voice.cpp:12](../src/swf/sound_voice.cpp#L12) |
| `FVM_SWFSoundVoiceInfo` | struct | [include/flashvm/swf_sound.h:83](../include/flashvm/swf_sound.h#L83) |
| `fvm_table_set_metatable` | function | [include/flashvm/table.h:30](../include/flashvm/table.h#L30) |
| `fvm_timers_poll` | function | [include/flashvm/timers.h:22](../include/flashvm/timers.h#L22) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
<!-- source-links:end -->
