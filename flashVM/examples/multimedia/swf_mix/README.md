# swf_mix — bounded prepared voices into one mixed PCM output

Sources: [main.c](main.c), [example_constants.h](example_constants.h).

## Назначение

Исполняемый C89-учебник готовит два native voice snapshots из встроенного FWS,
читает по два sample frames и суммирует их в один caller buffer. Первый голос
имеет три кадра, второй — три кадра и два повтора. После EOF короткого голоса
его вклад становится тишиной. Готовый mix передаётся через настоящий
fopen/fwrite/fclose output bridge, а не mock. Это пример собственного host mixer
pipeline без большого буфера на всю длительность и без устройства.

Часы задаёт host: одинаковый block offset, rate/channels обоих голосов. Сам
mixer не двигает snapshots и не исполняет stop/no-multiple. Контроль голосов,
реальное время, конверсия частоты/каналов и автоматическая MovieClip интеграция
требуют отдельного планировщика. Пример не воспроизводит слышимый звук.

## Подготовка, сборка, запуск

```sh
cmake -S . -B build-mix -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-mix --target flashvm_examples_multimedia_swf_mix_main --parallel
./build-mix/flashvm_examples_multimedia_swf_mix_main MIXED.pcm --interpreter
```

NO_MEDIA достаточен: внешние SDK, FFmpeg/ALSA, камера/микрофон и permissions не
нужны. Требуются C89/C++98 toolchain и writable output path. Названный файл может
быть перезаписан; без аргументов только usage, без VM/файлов. Native Windows/
macOS/Android paths используют стандартный stdio, но native platform acceptance
этим Linux-checkpoint не заявляется.

## Результат и реализация

```text
StopSounds execution: interpreter
Mixed 6 frames; expected six native S16 frames, no audible playback.
```

С ключом `--jit` первая строка — `StopSounds execution: native`; с
`--interpreter` — `interpreter`. Без ключа пример выбирает режим по [fvm_jit_available](../../../include/flashvm/jit.h#L13), создаёт обычную AVM1 функцию `{StopSounds, End}` и
вызывает её после последнего блока. StopSounds закрывает настоящий FILE через
зарегистрированный callback. Native path дополнительно требует ровно один
native_entries: скрытый fallback считается ошибкой. Для проверки этого пути
можно собрать с `cmake/profiles/minimal-jit.defs` и передать `--jit`.
Профиль INTERPRETER задаёт начальную политику, а не обязательно отсутствие
явного compiler API: доступность проверяется через [fvm_jit_available](../../../include/flashvm/jit.h#L13). Ни стандартные
AS объекты, ни отдельный аудиопоток для этого не требуются.

[GC:] Function handle используется до первого вызова без intervening collecting
callbacks; вызов устанавливает обычные activation roots. При расширении примера
с collecting host calls удерживайте функцию явным корнем. Automatic code cache
принадлежит context и уничтожается при его cleanup, отдельных JIT handles нет.

Файл содержит 12 bytes native-endian S16: `[-256, 0, -256, 32512, 0, -32768]`.
Длительность равна более длинному голосу (6), а не сумме длин (3+6).
Три пары caller buffers намеренно маленькие: source A заканчивается внутри
второго блока, source B продолжает второй проход. Все metadata проверяются до
mix write; saturate выполняется после суммы всех входов, без промежуточного
клиппинга, normalization или master gain.

[GC:] Snapshot storage — native copies без managed roots/context pointer. Host
владеет handles и закрывает их явно; VM не закрывает snapshots автоматически.
Context owns только регистрацию output, callback table копируется. Userdata
должна жить до возврата context_destroy. PCM borrowed только на время mix/write,
буферы переиспользуются после синхронного fwrite. При failed host write snapshots
уже продвинуты: пример сообщает ошибку, не повторяет chunk скрыто и делает cleanup.
Failed write может оставить FILE вне active state; fallback fclose после context
teardown закрывает такой ресурс. Runtime освобождается после context.

Для настоящего устройства замените FILE callbacks системным output bridge и
подавайте только совпадающую geometry. При разных rate/channel layouts сначала
явно выполните конверсию; mixer её не делает. Невыровненные input bytes допустимы,
выход — short-aligned. [ALSA preparation](../../../doc/ALSA_AUDIO_OUTPUT.md),
[написание мостов](../../../doc/MULTIMEDIA_BRIDGES.md).

## Демонстрируемые API

- [`fvm_audio_mix_s16`](../../../include/flashvm/multimedia.h#L131) — точная сумма, silence padding, лимиты/alias/ошибки:
  [AUDIO_MIXING.md](../../../doc/AUDIO_MIXING.md#fvm_audio_mix_s16).
- [`fvm_swf_sound_voice_open`](../../../include/flashvm/swf_sound.h#L82), [`fvm_swf_sound_voice_info`](../../../include/flashvm/swf_sound.h#L104),
  [`fvm_swf_sound_voice_read`](../../../include/flashvm/swf_sound.h#L106), [`fvm_swf_sound_voice_close`](../../../include/flashvm/swf_sound.h#L108) — ownership,
  progression, EOF and cleanup: [SWF_SOUND_VOICES.md](../../../doc/SWF_SOUND_VOICES.md).
- [`fvm_jit_available`](../../../include/flashvm/jit.h#L13), [`fvm_execution_set_mode`](../../../include/flashvm/execution_mode.h#L64),
  [`fvm_execution_get_stats`](../../../include/flashvm/execution_mode.h#L71), [`fvm_function_create_avm1`](../../../include/flashvm/function.h#L52), [`fvm_function_invoke`](../../../include/flashvm/function.h#L65) —
  выбор interpreter/automatic JIT, обычная активация StopSounds и проверка
  фактического native entry: [JIT_SOUND.md](../../../doc/JIT_SOUND.md),
  [EXECUTION_MODES.md](../../../doc/EXECUTION_MODES.md).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L82), [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L84)
  — copied host callbacks, actual synchronous output, retryable stop:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md).
- [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) — canonical input before native preparation:
  [SWF_SOUND_RESOURCES.md](../../../doc/SWF_SOUND_RESOURCES.md#scope--область-реализации).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68),
  [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — dependency-ordered VM lifetime:
  [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context),
  [terminal cleanup](../../../doc/AUDIO_CHANNELS.md#context-teardown).

## English

This fully line-commented C89 tutorial opens two native event snapshots, reads
small aligned blocks, sums them into a caller-owned S16 buffer and writes the
result through real stdio. The shorter voice becomes silence after EOF; the
longer voice repeats independently. The output is exactly six frames
[-256, 0, -256, 32512, 0, -32768], stored in native byte order.

The final stop is an ordinary AVM1 function containing StopSounds and End.
The tutorial selects automatic JIT when [fvm_jit_available](../../../include/flashvm/jit.h#L13) reports a working
backend, otherwise interpreter mode. Pass `--interpreter` or `--jit` after the
output path to select a specific path; unavailable JIT or an unknown switch is
rejected before VM/file creation. A native run requires exactly one actual
native entry from [fvm_execution_get_stats](../../../include/flashvm/execution_mode.h#L71); fallback fails the tutorial. Both
paths close the real FILE through the same guarded registered stop callback.
Build with minimal-jit.defs and pass `--jit` to exercise native execution.
An INTERPRETER build sets the initial policy; it need not disable explicit
compiler availability. The query, rather than the profile name, decides that. No standard objects
are required. The managed function is invoked before any collecting host call;
normal activation roots protect it during invocation. Add an explicit root if
extending preparation with collecting callbacks. The context owns automatic
compiled code and releases it at teardown.

Supply one explicit writable output path; an existing file may be replaced.
No-argument runs create no files or VM. NO_MEDIA needs no SDK/device. Source
geometry must match; there is no resampling, scheduling, automatic event control
or audible-playback claim. Snapshots are explicitly host-owned; output buffers
are synchronously borrowed and reused. After failed native output, already read
positions cannot roll back automatically. Cleanup closes snapshots, destroys the
context while userdata remains alive, recovers residual FILE ownership, then
releases runtime. API/source contracts are linked above.

Optional validation from the repository root:

```sh
python3 scripts/check_swf_mix_example.py build-mix/flashvm_examples_multimedia_swf_mix_main
python3 scripts/check_swf_mix_example.py build-mix/flashvm_examples_multimedia_swf_mix_main --ffmpeg
```

Optional checker `--native` / `--interpreter` flags select and require that exact observed execution
path. The second command additionally requires the system ffmpeg executable with amix;
it compares unnormalized longest-voice output byte-for-byte. Runtime mixing and
the tutorial need no FFmpeg. Проверка — отдельный oracle, не runtime dependency.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:82](../../../include/flashvm/multimedia.h#L82) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:84](../../../include/flashvm/multimedia.h#L84) |
| `fvm_audio_mix_s16` | function | [include/flashvm/multimedia.h:131](../../../include/flashvm/multimedia.h#L131) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_execution_get_stats` | function | [include/flashvm/execution_mode.h:71](../../../include/flashvm/execution_mode.h#L71) |
| `fvm_execution_set_mode` | function | [include/flashvm/execution_mode.h:64](../../../include/flashvm/execution_mode.h#L64) |
| `fvm_function_create_avm1` | function | [include/flashvm/function.h:52](../../../include/flashvm/function.h#L52) |
| `fvm_function_invoke` | function | [include/flashvm/function.h:65](../../../include/flashvm/function.h#L65) |
| `fvm_jit_available` | function | [include/flashvm/jit.h:13](../../../include/flashvm/jit.h#L13) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:108](../../../include/flashvm/swf_sound.h#L108) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:104](../../../include/flashvm/swf_sound.h#L104) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:82](../../../include/flashvm/swf_sound.h#L82) |
| `fvm_swf_sound_voice_read` | function | [include/flashvm/swf_sound.h:106](../../../include/flashvm/swf_sound.h#L106) |
<!-- source-links:end -->
