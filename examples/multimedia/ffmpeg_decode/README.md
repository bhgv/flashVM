# ffmpeg_decode — FFmpeg: декодирование файлов и явная камера

## Назначение и реализация / Purpose and implementation

Исполняемый пример вызывает настоящий backend из статических библиотек FFmpeg.
Режим audio выбирает один аудиопоток и возвращает S16 PCM; video — один видеопоток
и RGBA. Другие потоки пропускаются, временная шкала/синхронизация A/V не реализованы.
При первом буфере печатаются реальные формат/размер; в конце число буферов.
Воспроизведение/окно не создаются. Без MEDIA явный запуск сообщает UNSUPPORTED и
показывает настройку SDK; без аргументов выводится usage с успешным smoke exit.

Камера: `camera /dev/video0 video4linux2` на Linux с доступным V4L2 SDK и правами.
Открывается только явно заданное устройство и читается один кадр, затем всё
закрывается. Это реальный capture вызов, не автоматический тест камеры. Открытие
и read могут блокироваться; у checkpoint нет timeout/cancel API. Не используйте
этот режим в unattended smoke suite. Live камера в runner не проверена.

[GC:] Нативные session/buffers не входят в VM heap; [fvm_media_close](../../../include/flashvm/multimedia.h#L124) освобождает
libavformat/libavcodec/swscale storage после успеха или read failure. Ошибка open
не публикует session. Результат read — borrowed view до следующей операции.

English: real static library decode/capture, explicit source selection, no hidden
microphone/camera/output, bounded one-frame camera demo. Requires the MEDIA
profile and a target-matching SDK, not the ffmpeg executable.

## Сборка и запуск / Build and run

Все C89 примеры собираются в обычной CMake конфигурации; файл main.c — полностью
исполняемый учебник с комментариями каждой строки и без assertions. Для FFmpeg
сначала выполните [рецепт статического SDK](../../../external/ffmpeg/README.md).
Custom bridges работают и с NO_MEDIA. Linux event example требует Linux для
реального запуска с fd; заглушка другого target сообщает об отсутствии поддержки.

```sh
cmake -S . -B build-media -DFVM_DEFS_FILE=cmake/profiles/media.defs \
  -DFVM_FFMPEG_ROOT="$PWD/external/ffmpeg/install"
cmake --build build-media --target flashvm_examples_multimedia_ffmpeg_decode_main
./build-media/flashvm_examples_multimedia_ffmpeg_decode_main
./build-media/flashvm_examples_multimedia_ffmpeg_decode_main audio input.wav
```

Первый запуск без аргументов только печатает usage. Второй требует реальный
явно выбранный путь; замените placeholder своим. Не выдавайте отсутствие fixture
или физического устройства за успешную проверку работы устройства.

## Демонстрируемые API / Demonstrated API

| API (public source declaration is linked below) | Detailed contract and ownership guide |
| --- | --- |
| [`fvm_media_ffmpeg_backend`](../../../include/flashvm/multimedia.h#L107) | [MULTIMEDIA_BRIDGES.md — fvm_media_ffmpeg_backend](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_media_ffmpeg_backend) |
| [`fvm_audio_open`](../../../include/flashvm/multimedia.h#L110) | [MULTIMEDIA_BRIDGES.md — fvm_audio_open](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_audio_open) |
| [`fvm_video_open`](../../../include/flashvm/multimedia.h#L113) | [MULTIMEDIA_BRIDGES.md — fvm_video_open](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_video_open) |
| [`fvm_camera_open`](../../../include/flashvm/multimedia.h#L116) | [MULTIMEDIA_BRIDGES.md — fvm_camera_open](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_camera_open) |
| [`fvm_media_read`](../../../include/flashvm/multimedia.h#L119) | [MULTIMEDIA_BRIDGES.md — fvm_media_read](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_media_read) |
| [`fvm_media_close`](../../../include/flashvm/multimedia.h#L124) | [MULTIMEDIA_BRIDGES.md — fvm_media_close](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_media_close) |

Типы: [FVM_MediaBackend](../../../include/flashvm/multimedia.h#L55), [FVM_MediaSession](../../../src/multimedia/common/session.hpp#L5), [FVM_MediaFrame](../../../include/flashvm/multimedia.h#L36), [FVM_MediaOutput](../../../include/flashvm/multimedia.h#L66),
[FVM_DeviceBridge](../../../include/flashvm/multimedia.h#L100) и [FVM_DeviceEvent](../../../include/flashvm/multimedia.h#L91) описаны в
[общем руководстве](../../../doc/MULTIMEDIA_BRIDGES.md#общий-контракт--common-contract).
Ни одна демонстрация не завершает roadmap этап 4 или 9.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_audio_open` | function | [include/flashvm/multimedia.h:110](../../../include/flashvm/multimedia.h#L110) |
| `fvm_camera_open` | function | [include/flashvm/multimedia.h:116](../../../include/flashvm/multimedia.h#L116) |
| `FVM_DeviceBridge` | struct | [include/flashvm/multimedia.h:100](../../../include/flashvm/multimedia.h#L100) |
| `FVM_DeviceEvent` | struct | [include/flashvm/multimedia.h:91](../../../include/flashvm/multimedia.h#L91) |
| `fvm_media_close` | function | [include/flashvm/multimedia.h:124](../../../include/flashvm/multimedia.h#L124) |
| `fvm_media_ffmpeg_backend` | function | [include/flashvm/multimedia.h:107](../../../include/flashvm/multimedia.h#L107) |
| `fvm_media_read` | function | [include/flashvm/multimedia.h:119](../../../include/flashvm/multimedia.h#L119) |
| `FVM_MediaBackend` | struct | [include/flashvm/multimedia.h:55](../../../include/flashvm/multimedia.h#L55) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:36](../../../include/flashvm/multimedia.h#L36) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:66](../../../include/flashvm/multimedia.h#L66) |
| `FVM_MediaSession` | struct | [src/multimedia/common/session.hpp:5](../../../src/multimedia/common/session.hpp#L5) |
| `fvm_video_open` | function | [include/flashvm/multimedia.h:113](../../../include/flashvm/multimedia.h#L113) |
<!-- source-links:end -->
