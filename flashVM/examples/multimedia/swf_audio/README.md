# SWF DefineSound → PCM output / Явный экспорт звука SWF

Исполняемый C89-пример загружает явно названный FWS/CWS, перечисляет корневые
DefineSound, StartSound/SOUNDINFO и экспортирует один выбранный ID через настоящий файловый PCM sink.
Он полезен для проверки звуковых ресурсов и как шаблон соединения SWF-данных со
своей библиотекой: callback fwrite заменяется системным аудиовыходом, а fclose —
на stop/discard его очереди. Сам пример не воспроизводит слышимый звук.

## Сборка и запуск

```sh
cmake -S . -B build-swf-audio -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-swf-audio --target flashvm_examples_multimedia_swf_audio_main --parallel
./build-swf-audio/flashvm_examples_multimedia_swf_audio_main INPUT.swf 7 OUTPUT.pcm
```

Нужен существующий SWF с **8/16-bit PCM или SWF ADPCM DefineSound** ID 7; замените ID
на реально выбранный ненулевой UI16 key. Поддерживаются legacy native-order
format 0, SWF4+ little-endian format 3 и ADPCM format 1 (2–5 бит на код). Остальные сжатые кодеки печатают номинальные
метаданные, но submit возвращает UNSUPPORTED. Это не подтверждение их декодирования.
Без аргументов — только usage, файлов/устройств нет. С аргументами вход читается,
а output создаётся/перезаписывается только при первой поддерживаемой непустой
записи. Поэтому нулевой звук не создаёт файл. Выберите отдельный выходной путь.

Работает с NO_MEDIA, без FFmpeg/ALSA и сборки сторонних SDK. На Windows передайте
локальные пути; запуск там этим Linux-checkpoint не сертифицирован. Native-endian
signed-16 raw PCM не имеет WAV-заголовка: rate/channels выводятся перед экспортом.
При успехе размер output равен sample_count × channels × 2. Это файловый output,
не звуковая карта и не автоматический проигрыватель StartSound/timeline.

Сами действия из загруженного SWF не исполняются. Единственный запускаемый
bytecode — собственный короткий StopSounds учебника.

## Реализация и владение

[main.c](main.c) объясняет каждую непустую строку, без assert/abort. Вход ограничен
64 MiB и копируется в canonical VM storage через load_memory; исходный malloc
буфер после успешной загрузки сразу освобождается. Count/at показывают порядок
корневых определений; info требует уникальный выбранный ID. Ошибочные заголовки
и размер PCM обнаруживаются до вывода. Пример принимает только десятичные цифры
в ID и не обрезает переполнение.

Channel копирует таблицу, но host FILE/userdata остаются у программы до конца
context_destroy. Submit запрашивает по 256 sample frames и синхронно копирует их
перед callback. Это ограничивает временную память; текущий root scan повторяется
на запросах и не является real-time cache/mixer. По завершении Action 0x09
действительно вызывает host stop и fclose. Cleanup сначала уничтожает context,
затем закрывает любые неактивированные остаточные host ресурсы после failed write.
Runtime освобождается после context. Borrowed info.data не используется после reload.

Для своей ALSA-системы сначала вызовите open с rate/channels из поддерживаемого
sound info, зарегистрируйте полученный output и выберите размер chunk под её
buffer/clock policy. Provider держите до конца cleanup канала/контекста:
[ALSA preparation](../../../doc/ALSA_AUDIO_OUTPUT.md#build-and-endpoint-preparation).
Submit timestamp — source offset, не планировщик аудио. Host callback не сохраняет
borrowed кадр, не меняет реестр каналов и не запускает GC.

## StartSound inspection / разбор событий

Перед экспортом пример печатает все корневые StartSound: frame/ID/flags,
исходные in/out positions, loops и каждую точку огибающей. Проверяет полное число
ShowFrame, End и корректность SOUNDINFO; отсутствующий/дублированный sound ID или
повреждение завершает пример до создания output. Даже если в SWF есть SyncStop,
пример лишь печатает этот бит и затем явно экспортирует выбранный ID с начала
до конца; границы, повторы и огибающие не применяются. Для их воспроизведения
нужен отдельный host scheduler/mixer. Копии event/envelope не содержат borrowed
pointers, позиции огибающей имеют независимый 44100 Hz clock, gain 0..32768.

Before export the tutorial prints root StartSound/SOUNDINFO and every envelope
point. The root frame count and selected sound references must be valid. These
controls are inspection data; exported PCM still spans the selected whole sound
without applying stops, loops, in/out points or envelope gain.

## Демонстрируемые API

- [`fvm_swf_sound_event_count`](../../../include/flashvm/swf_sound.h#L56) — число корневых StartSound: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_event_count).
- [`fvm_swf_sound_event_at`](../../../include/flashvm/swf_sound.h#L58) — копия SOUNDINFO и разрешение ID: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_event_at).
- [`fvm_swf_sound_event_envelope`](../../../include/flashvm/swf_sound.h#L60) — копия точки и независимый clock: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_event_envelope).
- [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) — canonical FWS/CWS storage: [область API](../../../doc/SWF_SOUND_RESOURCES.md#scope--область-реализации).
- [`fvm_swf_sound_count`](../../../include/flashvm/swf_sound.h#L86), [`fvm_swf_sound_at`](../../../include/flashvm/swf_sound.h#L88) — подсчёт/порядок и ошибки дубликатов: [count](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_count), [at](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_at).
- [`fvm_swf_sound_info`](../../../include/flashvm/swf_sound.h#L90) — borrowed metadata и уникальный ID: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_info).
- [`fvm_swf_sound_submit`](../../../include/flashvm/swf_sound.h#L100) — явный диапазон и callback-independent PCM copy: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_submit).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L82) — регистрация настоящего output callback: [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_create).
- [`fvm_avm1_execute`](../../../include/flashvm/avm1.h#L38) — настоящий StopSounds, закрывающий sink: [семантика](../../../doc/AUDIO_CHANNELS.md#scope).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68), [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — VM ownership: [API index](../../../doc/API_INDEX.md#runtime-and-context), [terminal channel cleanup](../../../doc/AUDIO_CHANNELS.md#context-teardown).

Связанная функция, не вызываемая этим примером напрямую:
[`fvm_swf_sound_read_pcm`](../../../include/flashvm/swf_sound.h#L93) — извлечение в caller-owned short buffer:
[контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_read_pcm).

## English

A fully line-commented C89 executable enumerates root DefineSound resources from
an explicit FWS/CWS and exports one selected PCM or SWF ADPCM sound through real stdio
output. It opens no file/device without arguments, rejects invalid IDs and
oversized input, and writes raw native-endian S16 in explicit 256-frame chunks.
The named output is replaced when first used; an empty sound creates no file.
NO_MEDIA works without FFmpeg or ALSA. Other compressed codecs remain inspectable
metadata and return UNSUPPORTED for output. Host userdata outlives context teardown;
StopSounds closes the actual FILE. This is explicit resource export, not an
automatic timeline player, cached mixer or audible native-device acceptance.
ADPCM produces native S16 output. Each chunk revalidates the declared stream;
this export tutorial is not a cached real-time decoder. See the
[ADPCM contract](../../../doc/SWF_SOUND_RESOURCES.md#swf-adpcm--дифференциальное-аудио).
Eight-bit PCM format 0/3 is converted by `(byte - 128) * 256` to S16.
Восьмибитный PCM поддержан; итоговый файл по-прежнему содержит native S16.
## Styled event export / экспорт с SOUNDINFO

```sh
./build-swf-audio/flashvm_examples_multimedia_swf_audio_main INPUT.swf 0 EVENT.pcm --event
```

The explicit `--event` mode interprets the numeric selector as a zero-based
StartSound ordinal (parsed by strtoul, therefore bounded by unsigned long and
size_t). It resolves the event resource, then calls
[`fvm_swf_sound_event_submit`](../../../include/flashvm/swf_sound.h#L97) on a fresh dedicated inactive channel. In/out points,
loops and linear envelopes affect the file; SyncStop or an empty event creates
no output. StopSounds closes accepted output. A full prepared event must fit
the temporary memory limit; this mode is not chunked and may reject long loops.
The ordinary three-argument SOUND_ID mode retains its raw chunked behavior.

Режим `--event` использует ordinal с нуля вместо ID: применяет границы, повторы,
огибающую и передаёт PCM реальному fwrite одним буфером. Это один выделенный
голос, без автоматического MovieClip clock/mixing; stop/empty не создаёт файл.
Ошибки диапазона, порядка точек, кодека/памяти возникают до callback. Пример
показывает реальную запись файла, не слышимое воспроизведение и не полную
эквивалентность Flash Player. Новые каналы не демонстрируют активный no-multiple;
несколько голосов показаны в [swf_polyphony](../swf_polyphony/README.md),
его жизненный цикл — в [контракте](../../../doc/SWF_SOUND_PLAYBACK.md).

- [`fvm_swf_sound_event_submit`](../../../include/flashvm/swf_sound.h#L97) — отдельный готовый голос, ownership, ошибки,
  стиль и ограничения: [SWF_SOUND_PLAYBACK.md](../../../doc/SWF_SOUND_PLAYBACK.md#fvm_swf_sound_event_submit).
## Incremental prepared voice / порционное чтение

```sh
./build-swf-audio/flashvm_examples_multimedia_swf_audio_main INPUT.swf 0 OUTPUT.pcm --stream-event
```

Prepare the selected StartSound once, then read at most 256 frames into a caller
buffer and pass initialized values through the actual file-output channel. The
snapshot stores source PCM/envelope once even for long loops. The source itself
must still fit the native budget. Close the snapshot explicitly; it is independent
of context lifetime. EOF is NOT_FOUND, the final successful chunk may be short.
If a native write fails the reader has already advanced; this example aborts
normally rather than replaying or silently skipping the failed chunk.

This mode renders styles but does not execute stop/no-multiple or bind a resource
to its output channel. Stop controls return UNSUPPORTED before file creation;
use --event for synchronization. Empty/zero-play snapshots create no file.
The channel remains ordinary host output and StopSounds closes it explicitly.
No automatic timeline, mixer or SoundStreamBlock decoding is introduced.

Режим --stream-event готовит source/envelope один раз, читает порциями до 256
кадров без дальнейших аллокаций/разбора SWF и передаёт S16 реальному fwrite.
Последний chunk может быть короче, EOF — NOT_FOUND. Handle нужно закрыть явно,
он не зависит от context. Отклоняются stop-controls, no-multiple не исполняется.
При failed write chunk уже прочитан: пример сообщает ошибку и выполняет cleanup.
Обычные --event и SOUND_ID сохраняют прежние контракты. Лимиты/размер chunk
объявлены в [example_constants.h](example_constants.h).

- [`fvm_swf_sound_voice_open`](../../../include/flashvm/swf_sound.h#L78) — native ownership, budget and errors:
  [SWF_SOUND_VOICES.md](../../../doc/SWF_SOUND_VOICES.md#fvm_swf_sound_voice_open).
- [`fvm_swf_sound_voice_read`](../../../include/flashvm/swf_sound.h#L82) — bounded S16, EOF and advancement:
  [contract](../../../doc/SWF_SOUND_VOICES.md#fvm_swf_sound_voice_read).
- [`fvm_swf_sound_voice_info`](../../../include/flashvm/swf_sound.h#L80) — copied geometry/progress and exact struct_size:
  [contract](../../../doc/SWF_SOUND_VOICES.md#fvm_swf_sound_voice_info).
- [`fvm_swf_sound_voice_close`](../../../include/flashvm/swf_sound.h#L84) — explicit VM-independent native cleanup:
  [contract](../../../doc/SWF_SOUND_VOICES.md#fvm_swf_sound_voice_close).
- [`fvm_audio_channel_write`](../../../include/flashvm/multimedia.h#L84) — synchronous ordinary output for each caller chunk:
  [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_write).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:82](../../../include/flashvm/multimedia.h#L82) |
| `fvm_audio_channel_write` | function | [include/flashvm/multimedia.h:84](../../../include/flashvm/multimedia.h#L84) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:38](../../../include/flashvm/avm1.h#L38) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_sound_at` | function | [include/flashvm/swf_sound.h:88](../../../include/flashvm/swf_sound.h#L88) |
| `fvm_swf_sound_count` | function | [include/flashvm/swf_sound.h:86](../../../include/flashvm/swf_sound.h#L86) |
| `fvm_swf_sound_event_at` | function | [include/flashvm/swf_sound.h:58](../../../include/flashvm/swf_sound.h#L58) |
| `fvm_swf_sound_event_count` | function | [include/flashvm/swf_sound.h:56](../../../include/flashvm/swf_sound.h#L56) |
| `fvm_swf_sound_event_envelope` | function | [include/flashvm/swf_sound.h:60](../../../include/flashvm/swf_sound.h#L60) |
| `fvm_swf_sound_event_submit` | function | [include/flashvm/swf_sound.h:97](../../../include/flashvm/swf_sound.h#L97) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:90](../../../include/flashvm/swf_sound.h#L90) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:93](../../../include/flashvm/swf_sound.h#L93) |
| `fvm_swf_sound_submit` | function | [include/flashvm/swf_sound.h:100](../../../include/flashvm/swf_sound.h#L100) |
| `fvm_swf_sound_voice_close` | function | [include/flashvm/swf_sound.h:84](../../../include/flashvm/swf_sound.h#L84) |
| `fvm_swf_sound_voice_info` | function | [include/flashvm/swf_sound.h:80](../../../include/flashvm/swf_sound.h#L80) |
| `fvm_swf_sound_voice_open` | function | [include/flashvm/swf_sound.h:78](../../../include/flashvm/swf_sound.h#L78) |
| `fvm_swf_sound_voice_read` | function | [include/flashvm/swf_sound.h:82](../../../include/flashvm/swf_sound.h#L82) |
<!-- source-links:end -->
