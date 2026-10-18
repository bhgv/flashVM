# system_devices — Свой опрос устройств через Linux evdev

## Назначение и реализация / Purpose and implementation

Показывает адаптер реальной системной очереди к [FVM_DeviceBridge](../../../include/flashvm/multimedia.h#L102).poll. Main явно
открывает указанный /dev/input/eventN с O_RDONLY|O_NONBLOCK, хранит fd и передаёт
его адрес через userdata. Callback не владеет fd и делегирует read нативному
переходнику. После максимум 64 polling calls main закрывает fd и проверяет close.
Нет ожидания, фонового потока, изменения прав или поиска устройств.

Печатаются raw kind/native_code/value/time. Отсутствие вывода при пустой очереди
нормально. EV_KEY включает мышиные кнопки, EV_REL даёт deltas, EV_ABS — axis/slot/
contact fragments. Не производится реконструкция touch gestures или перевод в
координаты сцены: host хранит slots/SYN, масштабирует и вызывает VM input API на
VM потоке. Другие ОС получают честное сообщение о неприменимости Linux-примера.

[GC:] fd и event принадлежат host, VM не создаётся. Для датчика заменить callback
реальным SDK чтением и вернуть полную XYZ выборку в оговорённых единицах.
Compass/accelerometer драйвер в этом примере не имитируется.

English: explicit host-owned nonblocking device handle, bounded polling, real
Linux read calls, raw codes/units, ordinary permission/I/O failures and cleanup.
No physical hardware result is guaranteed by the pipe-based regression fixture.

## Сборка и запуск / Build and run

Все C89 примеры собираются в обычной CMake конфигурации; файл main.c — полностью
исполняемый учебник с комментариями каждой строки и без assertions. Для FFmpeg
сначала выполните [рецепт статического SDK](../../../external/ffmpeg/README.md).
Custom bridges работают и с NO_MEDIA. Linux event example требует Linux для
реального запуска с fd; заглушка другого target сообщает об отсутствии поддержки.

```sh
cmake -S . -B build-media -DFVM_DEFS_FILE=cmake/profiles/media.defs \
  -DFVM_FFMPEG_ROOT="$PWD/external/ffmpeg/install"
cmake --build build-media --target flashvm_examples_multimedia_system_devices_main
./build-media/flashvm_examples_multimedia_system_devices_main
./build-media/flashvm_examples_multimedia_system_devices_main /dev/input/eventN
```

Первый запуск без аргументов только печатает usage. Второй требует реальный
явно выбранный путь; замените placeholder своим. Не выдавайте отсутствие fixture
или физического устройства за успешную проверку работы устройства.

## Демонстрируемые API / Demonstrated API

| API (public source declaration is linked below) | Detailed contract and ownership guide |
| --- | --- |
| [`fvm_device_poll`](../../../include/flashvm/multimedia.h#L140) | [MULTIMEDIA_BRIDGES.md — fvm_device_poll](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_device_poll) |
| [`fvm_input_linux_evdev_read`](../../../include/flashvm/multimedia.h#L143) | [MULTIMEDIA_BRIDGES.md — fvm_input_linux_evdev_read](../../../doc/MULTIMEDIA_BRIDGES.md#fvm_input_linux_evdev_read) |

Типы: [FVM_MediaBackend](../../../include/flashvm/multimedia.h#L57), [FVM_MediaSession](../../../src/multimedia/common/session.hpp#L5), [FVM_MediaFrame](../../../include/flashvm/multimedia.h#L38), [FVM_MediaOutput](../../../include/flashvm/multimedia.h#L68),
[FVM_DeviceBridge](../../../include/flashvm/multimedia.h#L102) и [FVM_DeviceEvent](../../../include/flashvm/multimedia.h#L93) описаны в
[общем руководстве](../../../doc/MULTIMEDIA_BRIDGES.md#общий-контракт--common-contract).
Ни одна демонстрация не завершает roadmap этап 4 или 9.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_device_poll` | function | [include/flashvm/multimedia.h:140](../../../include/flashvm/multimedia.h#L140) |
| `FVM_DeviceBridge` | struct | [include/flashvm/multimedia.h:102](../../../include/flashvm/multimedia.h#L102) |
| `FVM_DeviceEvent` | struct | [include/flashvm/multimedia.h:93](../../../include/flashvm/multimedia.h#L93) |
| `fvm_input_linux_evdev_read` | function | [include/flashvm/multimedia.h:143](../../../include/flashvm/multimedia.h#L143) |
| `FVM_MediaBackend` | struct | [include/flashvm/multimedia.h:57](../../../include/flashvm/multimedia.h#L57) |
| `FVM_MediaFrame` | struct | [include/flashvm/multimedia.h:38](../../../include/flashvm/multimedia.h#L38) |
| `FVM_MediaOutput` | struct | [include/flashvm/multimedia.h:68](../../../include/flashvm/multimedia.h#L68) |
| `FVM_MediaSession` | struct | [src/multimedia/common/session.hpp:5](../../../src/multimedia/common/session.hpp#L5) |
<!-- source-links:end -->
