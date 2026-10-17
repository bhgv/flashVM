# swf_polyphony — independent StartSound voice synchronization

## Назначение

Исходники: [main.c](main.c), [example_constants.h](example_constants.h).

Исполняемый C89-учебник создаёт два голоса одного DefineSound на разных каналах,
проверяет no-multiple через третий незакреплённый канал, затем использует его для
другого ресурса. SyncStop первого ресурса закрывает оба его выхода, сохраняя
другой ресурс активным. Реальные callbacks вызывают fopen/fwrite/fclose; это не
заглушка и не слышимое воспроизведение. Небольшой literal SWF встроен в main.c:
два U8 sample frames, два resource ID и четыре StartSound в одном кадре.

Это пример управления несколькими голосами и собственного output bridge. Он не
смешивает их: системный audio mixer/выход для каждого голоса host предоставляет
сам. Отдельные FILE делают независимость каналов наблюдаемой без устройства.
События выбираются явно, автоматического MovieClip clock нет.

## Сборка, подготовка и запуск

```sh
cmake -S . -B build-polyphony -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-polyphony --target flashvm_examples_multimedia_swf_polyphony_main --parallel
./build-polyphony/flashvm_examples_multimedia_swf_polyphony_main A.pcm B.pcm OTHER.pcm
```

NO_MEDIA достаточен: FFmpeg/ALSA, камера, микрофон и разрешения не требуются.
Нужна writable папка и три явно выбранных разных файла: существующие файлы
перезаписываются. Не используйте разные aliases/symlinks одного и того же файла;
пример отклоняет лишь одинаковые строки пути. Без аргументов только usage,
файлы и VM не создаются. На Windows используются обычные пути/stdio; native
Windows/macOS/Android run этим Linux-checkpoint не подтверждается.

## Ожидаемый результат

```text
After no-multiple: A=1 writes, B=1 writes, unused=0 writes
After resource stop: A=1 stops, B=1 stops, other active=1
```

Каждый из трёх файлов содержит 4 байта native-endian S16: -32768, 32512,
преобразованные из U8 0/255. В момент no-multiple третий файл ещё не открыт и
канал не закреплён; последующий другой ID может его использовать. SyncStop
закрывает A/B; общий stop_all закрывает OTHER. Физическое одновременное звучание
и микширование этим файловым примером не проверяются.

## Ownership и реализация

[VoiceFile](main.c#L7) — host-owned userdata; живёт до возврата context_destroy. Регистрация
копирует callback table, не забирая FILE/path. Первое успешное event write
закрепляет ID за каналом до close/context destruction; failed write не закрепляет.
No-multiple ищет только active bound same-ID голоса текущего контекста и не
закрепляет пропущенный target. SyncStop пытается остановить все совпадения в
порядке регистрации, продолжает после ошибок и возвращает первую. Неудачно
остановленные остаются active для retry, успешные inactive. Другие ID, unbound
host outputs и контексты не затрагиваются.

Буфер callback borrowed только во время write, поэтому fwrite выполняется сразу.
На ошибке FILE сохраняется для cleanup. fclose обнуляет handle даже при ошибке:
повторный stop безопасен. Context уничтожается до runtime и до fallback fclose;
cleanup освобождает FILE, оставшиеся после failed writes вне active state.
[GC:] userdata не является managed объектом; native output barrier запрещает
GC и повторную мутацию audio registry внутри callback. Для настоящего выхода
замените FILE operations системным bridge с независимым voice state; подготовка
ALSA описана в [ALSA_AUDIO_OUTPUT.md](../../../doc/ALSA_AUDIO_OUTPUT.md).

## Демонстрируемые API и руководство

- [`fvm_swf_sound_event_submit`](../../../include/flashvm/swf_sound.h#L97) — overlap, global no-multiple и stop-by-ID:
  [SWF_SOUND_PLAYBACK.md](../../../doc/SWF_SOUND_PLAYBACK.md#fvm_swf_sound_event_submit).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L80) — copied registration и host ownership:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_create).
- [`fvm_audio_channel_active`](../../../include/flashvm/multimedia.h#L88) — copied state без host callback:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_active).
- [`fvm_audio_stop_all`](../../../include/flashvm/multimedia.h#L90) — оставшийся output и error/retry policy:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_stop_all).
- [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) — canonical SWF storage:
  [SWF_SOUND_RESOURCES.md](../../../doc/SWF_SOUND_RESOURCES.md#scope--область-реализации).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68),
  [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — lifetime:
  [API_INDEX.md](../../../doc/API_INDEX.md#runtime-and-context),
  [terminal cleanup](../../../doc/AUDIO_CHANNELS.md#context-teardown).

## English

A fully line-commented C89 executable creates two bound resource-7 voices,
suppresses another instance on a fresh target through context-wide no-multiple,
then reuses that still-unbound target for resource 8. Resource-7 SyncStop closes
both matching real FILE outputs and leaves resource 8 active; stop_all closes
the remainder. Each resulting file contains native S16 [-32768, 32512].

Supply three explicit distinct writable paths; files may be replaced. No-argument
runs only print usage. NO_MEDIA needs no SDK/device. Each output owns independent
native state; tables are copied, userdata remains host-owned through context
cleanup. Failed writes retain FILE for fallback cleanup; context must die before
runtime and userdata. Actual system audio requires a host mixer/output bridge;
this fixture validates native stdio and voice synchronization, not audible
mixing, automatic timeline scheduling or foreign-platform acceptance. API links
above name exact guide contracts and source declarations.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_active` | function | [include/flashvm/multimedia.h:88](../../../include/flashvm/multimedia.h#L88) |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:80](../../../include/flashvm/multimedia.h#L80) |
| `fvm_audio_stop_all` | function | [include/flashvm/multimedia.h:90](../../../include/flashvm/multimedia.h#L90) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:97](../../../include/flashvm/swf_sound.h#L97) |
<!-- source-links:end -->
