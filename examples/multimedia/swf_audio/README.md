# SWF DefineSound → PCM output / Явный экспорт звука SWF

Исполняемый C89-пример загружает явно названный FWS/CWS, перечисляет корневые
DefineSound и экспортирует один выбранный ID через настоящий файловый PCM sink.
Он полезен для проверки звуковых ресурсов и как шаблон соединения SWF-данных со
своей библиотекой: callback fwrite заменяется системным аудиовыходом, а fclose —
на stop/discard его очереди. Сам пример не воспроизводит слышимый звук.

## Сборка и запуск

```sh
cmake -S . -B build-swf-audio -DFVM_DEFS_FILE=cmake/profiles/minimal.defs -DCMAKE_BUILD_TYPE=Release
cmake --build build-swf-audio --target flashvm_examples_multimedia_swf_audio_main --parallel
./build-swf-audio/flashvm_examples_multimedia_swf_audio_main INPUT.swf 7 OUTPUT.pcm
```

Нужен существующий SWF с **16-bit uncompressed DefineSound** ID 7; замените ID
на реально выбранный ненулевой UI16 key. Поддерживаются legacy native-order
format 0 и SWF4+ little-endian format 3. Другие кодеки/8-bit печатают номинальные
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

## Демонстрируемые API

- [`fvm_load_memory`](../../../include/flashvm/flashvm.h#L72) — canonical FWS/CWS storage: [область API](../../../doc/SWF_SOUND_RESOURCES.md#scope--область-реализации).
- [`fvm_swf_sound_count`](../../../include/flashvm/swf_sound.h#L26), [`fvm_swf_sound_at`](../../../include/flashvm/swf_sound.h#L28) — подсчёт/порядок и ошибки дубликатов: [count](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_count), [at](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_at).
- [`fvm_swf_sound_info`](../../../include/flashvm/swf_sound.h#L30) — borrowed metadata и уникальный ID: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_info).
- [`fvm_swf_sound_submit`](../../../include/flashvm/swf_sound.h#L37) — явный диапазон и callback-independent PCM copy: [контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_submit).
- [`fvm_audio_channel_create`](../../../include/flashvm/multimedia.h#L80) — регистрация настоящего output callback: [AUDIO_CHANNELS.md](../../../doc/AUDIO_CHANNELS.md#fvm_audio_channel_create).
- [`fvm_avm1_execute`](../../../include/flashvm/avm1.h#L38) — настоящий StopSounds, закрывающий sink: [семантика](../../../doc/AUDIO_CHANNELS.md#scope).
- [`fvm_config_init`](../../../include/flashvm/flashvm.h#L62), [`fvm_runtime_create`](../../../include/flashvm/flashvm.h#L64), [`fvm_context_create`](../../../include/flashvm/flashvm.h#L68), [`fvm_context_destroy`](../../../include/flashvm/flashvm.h#L70), [`fvm_runtime_destroy`](../../../include/flashvm/flashvm.h#L66) — VM ownership: [API index](../../../doc/API_INDEX.md#runtime-and-context), [terminal channel cleanup](../../../doc/AUDIO_CHANNELS.md#context-teardown).

Связанная функция, не вызываемая этим примером напрямую:
[`fvm_swf_sound_read_pcm`](../../../include/flashvm/swf_sound.h#L33) — извлечение в caller-owned short buffer:
[контракт](../../../doc/SWF_SOUND_RESOURCES.md#fvm_swf_sound_read_pcm).

## English

A fully line-commented C89 executable enumerates root DefineSound resources from
an explicit FWS/CWS and exports one selected 16-bit PCM sound through real stdio
output. It opens no file/device without arguments, rejects invalid IDs and
oversized input, and writes raw native-endian S16 in explicit 256-frame chunks.
The named output is replaced when first used; an empty sound creates no file.
NO_MEDIA works without FFmpeg or ALSA. Unsupported codecs/8-bit remain inspectable
metadata and return UNSUPPORTED for output. Host userdata outlives context teardown;
StopSounds closes the actual FILE. This is explicit resource export, not an
automatic timeline player, cached mixer or audible native-device acceptance.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_channel_create` | function | [include/flashvm/multimedia.h:80](../../../include/flashvm/multimedia.h#L80) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:38](../../../include/flashvm/avm1.h#L38) |
| `fvm_config_init` | function | [include/flashvm/flashvm.h:62](../../../include/flashvm/flashvm.h#L62) |
| `fvm_context_create` | function | [include/flashvm/flashvm.h:68](../../../include/flashvm/flashvm.h#L68) |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../../../include/flashvm/flashvm.h#L70) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../../../include/flashvm/flashvm.h#L72) |
| `fvm_runtime_create` | function | [include/flashvm/flashvm.h:64](../../../include/flashvm/flashvm.h#L64) |
| `fvm_runtime_destroy` | function | [include/flashvm/flashvm.h:66](../../../include/flashvm/flashvm.h#L66) |
| `fvm_swf_sound_at` | function | [include/flashvm/swf_sound.h:28](../../../include/flashvm/swf_sound.h#L28) |
| `fvm_swf_sound_count` | function | [include/flashvm/swf_sound.h:26](../../../include/flashvm/swf_sound.h#L26) |
| `fvm_swf_sound_info` | function | [include/flashvm/swf_sound.h:30](../../../include/flashvm/swf_sound.h#L30) |
| `fvm_swf_sound_read_pcm` | function | [include/flashvm/swf_sound.h:33](../../../include/flashvm/swf_sound.h#L33) |
| `fvm_swf_sound_submit` | function | [include/flashvm/swf_sound.h:37](../../../include/flashvm/swf_sound.h#L37) |
<!-- source-links:end -->
