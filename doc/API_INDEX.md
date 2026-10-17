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

[`fvm_movieclip_swf_document_info`](../include/flashvm/movieclip.h#L25) — [SWF document metadata / метаданные SWF](SWF_DOCUMENT_METADATA.md), `include/flashvm/movieclip.h`.

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
Context audio registration and script stopping: [AUDIO_CHANNELS.md](AUDIO_CHANNELS.md).
[`fvm_audio_channel_create`](../include/flashvm/multimedia.h#L80), [`fvm_audio_channel_write`](../include/flashvm/multimedia.h#L82), [`fvm_audio_channel_stop`](../include/flashvm/multimedia.h#L84),
[`fvm_audio_channel_close`](../include/flashvm/multimedia.h#L86), [`fvm_audio_channel_active`](../include/flashvm/multimedia.h#L88) and [`fvm_audio_stop_all`](../include/flashvm/multimedia.h#L90)
provide copied output tables, explicit ownership and shared StopSounds semantics.

Native Linux output: [`fvm_audio_linux_alsa_open`](../include/flashvm/multimedia.h#L75), [`fvm_audio_linux_alsa_close`](../include/flashvm/multimedia.h#L77) —
[static ALSA provider contracts](ALSA_AUDIO_OUTPUT.md).

## Speech / Речевые службы

[Detailed TTS/STT API and per-OS preparation](SPEECH_BRIDGES.md); [public speech.h](../include/flashvm/speech.h).
Separate explicit voice and microphone sessions, replaceable native providers, retained UTF-8 transcript query/copy.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_active` | function | [include/flashvm/multimedia.h:88](../include/flashvm/multimedia.h#L88) |
| `fvm_audio_channel_close` | function | [include/flashvm/multimedia.h:86](../include/flashvm/multimedia.h#L86) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:80](../include/flashvm/multimedia.h#L80) |
| `fvm_audio_channel_stop` | function | [include/flashvm/multimedia.h:84](../include/flashvm/multimedia.h#L84) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:82](../include/flashvm/multimedia.h#L82) |
| `fvm_audio_linux_alsa_close` | function | [include/flashvm/multimedia.h:77](../include/flashvm/multimedia.h#L77) |
| `fvm_audio_linux_alsa_open` | function | [include/flashvm/multimedia.h:75](../include/flashvm/multimedia.h#L75) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:90](../include/flashvm/multimedia.h#L90) |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_Config` | struct | [include/flashvm/flashvm.h:51](../include/flashvm/flashvm.h#L51) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
| `fvm_meta_call` | function | [include/flashvm/meta_call.h:20](../include/flashvm/meta_call.h#L20) |
| `fvm_meta_register` | function | [include/flashvm/metamethod.h:20](../include/flashvm/metamethod.h#L20) |
| `fvm_movieclip_swf_document_info` | function | [include/flashvm/movieclip.h:25](../include/flashvm/movieclip.h#L25) |
| `FVM_Object` | struct | [src/core/runtime.hpp:118](../src/core/runtime.hpp#L118) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
| `fvm_register_function` | function | [include/flashvm/value.h:101](../include/flashvm/value.h#L101) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:40](../include/flashvm/flashvm.h#L40) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../include/flashvm/flashvm.h#L66) |
| `fvm_stage_get_quality` | function | [include/flashvm/stage.h:19](../include/flashvm/stage.h#L19) |
| `fvm_stage_set_quality` | function | [include/flashvm/stage.h:22](../include/flashvm/stage.h#L22) |
| `fvm_stage_toggle_quality` | function | [include/flashvm/stage.h:25](../include/flashvm/stage.h#L25) |
| `fvm_table_set_metatable` | function | [include/flashvm/table.h:24](../include/flashvm/table.h#L24) |
| `FVM_Value` | struct | [include/flashvm/value.h:24](../include/flashvm/value.h#L24) |
<!-- source-links:end -->
