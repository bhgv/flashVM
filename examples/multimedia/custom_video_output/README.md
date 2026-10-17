# custom_video_output — Свой вывод RGBA в реальный PPM файл

## Назначение и реализация / Purpose and implementation

Показывает написание [FVM_MediaOutput](../../../include/flashvm/multimedia.h#L66).write поверх реальных fprintf/fwrite.
Стековый RGBA8 кадр 2×1 содержит красный и зелёный пиксели. [fvm_video_present](../../../include/flashvm/multimedia.h#L131)
проверяет размер/stride; callback пишет PPM P6 и отбрасывает alpha. Файл создаётся
только по явному аргументу; существующий файл по этому пути заменяется. Временный
буфер не сохраняется после возврата callback. FILE принадлежит main и закрывается
даже при неуспешной записи; проверяется также ошибка flush/close.

Для настоящего окна заменить реализацию write системной библиотекой; при
асинхронной передаче скопировать borrowed bytes в принадлежащую очереди память.
Этот пример пишет файл, не открывает окно. [GC:] Здесь нет VM объектов или корней.

English: actual synchronous file-output bridge, geometry validation, row-stride
handling, caller-owned FILE and explicit destination. Expected artifact is a
2×1 PPM image, not a claim of native window support.

## Сборка и запуск / Build and run

Все C89 примеры собираются в обычной CMake конфигурации; файл main.c — полностью
исполняемый учебник с комментариями каждой строки и без assertions. Для FFmpeg
сначала выполните [рецепт статического SDK](../../../external/ffmpeg/README.md).
Custom bridges работают и с NO_MEDIA. Linux event example требует Linux для
реального запуска с fd; заглушка другого target сообщает об отсутствии поддержки.

```sh
cmake -S . -B build-media -DFVM_DEFS_FILE=cmake/profiles/media.defs \
  -DFVM_FFMPEG_ROOT="$PWD/external/ffmpeg/install"
cmake --build build-media --target flashvm_examples_multimedia_custom_video_output_main
./build-media/flashvm_examples_multimedia_custom_video_output_main
./build-media/flashvm_examples_multimedia_custom_video_output_main output.ppm
```

Первый запуск без аргументов только печатает usage. Второй требует реальный
явно выбранный путь; замените placeholder своим. Не выдавайте отсутствие fixture
или физического устройства за успешную проверку работы устройства.

## Демонстрируемые API / Demonstrated API

| API (public source declaration is linked below) | Detailed contract and ownership guide |
| --- | --- |
| [`fvm_video_present`](../../../include/flashvm/multimedia.h#L131) | [MULTIMEDIA_BRIDGES.md — fvm_video_present](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_video_present) |

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
| `FVM_DeviceBridge` | struct | [include/flashvm/multimedia.h:100](../../../include/flashvm/multimedia.h#L100) |
| `FVM_DeviceEvent` | struct | [include/flashvm/multimedia.h:91](../../../include/flashvm/multimedia.h#L91) |
| `FVM_MediaBackend` | struct | [include/flashvm/multimedia.h:55](../../../include/flashvm/multimedia.h#L55) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:36](../../../include/flashvm/multimedia.h#L36) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:66](../../../include/flashvm/multimedia.h#L66) |
| `FVM_MediaSession` | struct | [src/multimedia/common/session.hpp:5](../../../src/multimedia/common/session.hpp#L5) |
| `fvm_video_present` | function | [include/flashvm/multimedia.h:131](../../../include/flashvm/multimedia.h#L131) |
<!-- source-links:end -->
