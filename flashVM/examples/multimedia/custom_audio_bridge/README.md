# custom_audio_bridge — Свой источник PCM через stdio

## Назначение и реализация / Purpose and implementation

Реальный custom backend читает явно заданный raw PCM файл через fopen/fread.
Это пример замены FFmpeg на собственную библиотеку/системный источник, а не
воспроизведение на звуковую карту. Формат: mono, 8000 Hz, native-endian signed
16-bit; заголовка нет. Неполный последний sample считается ошибкой. Backend
реализует open/read/close, оставляет seek null. У каждого session собственный FILE
и буфер. Ошибка open освобождает частичное состояние сама; успешный close закрывает
FILE и освобождает malloc. В session копируется таблица, не userdata.

Буфер читается как borrowed view: следующий read делает предыдущий указатель
недействительным. Пример сохраняет только число sample frames. [GC:] Ресурсы не
принадлежат GC; VM объекты не создаются. Нативный stdout сообщает число прочитанных
sample frames; ошибок и assertions в учебном потоке не скрываем.

English: replaceable real stdio backend, explicit raw format, per-session native
ownership, synchronous borrowed buffers, ordinary EOF/error handling. No VM or
playback device is created.

## Сборка и запуск / Build and run

Все C89 примеры собираются в обычной CMake конфигурации; файл main.c — полностью
исполняемый учебник с комментариями каждой строки и без assertions. Для FFmpeg
сначала выполните [рецепт статического SDK](../../../external/ffmpeg/README.md).
Custom bridges работают и с NO_MEDIA. Linux event example требует Linux для
реального запуска с fd; заглушка другого target сообщает об отсутствии поддержки.

```sh
cmake -S . -B build-media -DFVM_DEFS_FILE=cmake/profiles/media.defs \
  -DFVM_FFMPEG_ROOT="$PWD/external/ffmpeg/install"
cmake --build build-media --target flashvm_examples_multimedia_custom_audio_bridge_main
./build-media/flashvm_examples_multimedia_custom_audio_bridge_main
./build-media/flashvm_examples_multimedia_custom_audio_bridge_main input-native-s16-mono-8000.raw
```

Первый запуск без аргументов только печатает usage. Второй требует реальный
явно выбранный путь; замените placeholder своим. Не выдавайте отсутствие fixture
или физического устройства за успешную проверку работы устройства.

## Демонстрируемые API / Demonstrated API

| API (public source declaration is linked below) | Detailed contract and ownership guide |
| --- | --- |
| [`fvm_audio_open`](../../../include/flashvm/multimedia.h#L111) | [MULTIMEDIA_BRIDGES.md — fvm_audio_open](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_audio_open) |
| [`fvm_media_read`](../../../include/flashvm/multimedia.h#L120) | [MULTIMEDIA_BRIDGES.md — fvm_media_read](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_media_read) |
| [`fvm_media_close`](../../../include/flashvm/multimedia.h#L125) | [MULTIMEDIA_BRIDGES.md — fvm_media_close](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_media_close) |

Типы: [FVM_MediaBackend](../../../include/flashvm/multimedia.h#L56), [FVM_MediaSession](../../../src/multimedia/common/session.hpp#L5), [FVM_MediaFrame](../../../include/flashvm/multimedia.h#L37), [FVM_MediaOutput](../../../include/flashvm/multimedia.h#L67),
[FVM_DeviceBridge](../../../include/flashvm/multimedia.h#L101) и [FVM_DeviceEvent](../../../include/flashvm/multimedia.h#L92) описаны в
[общем руководстве](../../../doc/MULTIMEDIA_BRIDGES.md#общий-контракт--common-contract).
Ни одна демонстрация не завершает roadmap этап 4 или 9.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_open` | function | [include/flashvm/multimedia.h:111](../../../include/flashvm/multimedia.h#L111) |
| `FVM_DeviceBridge` | struct | [include/flashvm/multimedia.h:101](../../../include/flashvm/multimedia.h#L101) |
| `FVM_DeviceEvent` | struct | [include/flashvm/multimedia.h:92](../../../include/flashvm/multimedia.h#L92) |
| `fvm_media_close` | function | [include/flashvm/multimedia.h:125](../../../include/flashvm/multimedia.h#L125) |
| `fvm_media_read` | function | [include/flashvm/multimedia.h:120](../../../include/flashvm/multimedia.h#L120) |
| `FVM_MediaBackend` | struct | [include/flashvm/multimedia.h:56](../../../include/flashvm/multimedia.h#L56) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:37](../../../include/flashvm/multimedia.h#L37) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:67](../../../include/flashvm/multimedia.h#L67) |
| `FVM_MediaSession` | struct | [src/multimedia/common/session.hpp:5](../../../src/multimedia/common/session.hpp#L5) |
<!-- source-links:end -->
