# PNG, GIF и JPEG в дочернем клипе

[English](README.md) · [Каталог](../../README.md#example-player-image_loading) ·
[Контракт](../../../doc/IMAGE_LOADING.ru.md).

Построчно прокомментированный [C89](main.c) запускает готовые
[PNG](png-start.swf)/[GIF](gif-start.swf)/[JPEG](jpeg-start.swf) сценарии.
Проверяются размеры через ActionGetProperty, удаление старого ребёнка,
идентичность, собственные данные, unload и реальное исполнение [SWF](after.swf).
Неверный/пропавший результат даёт ошибку ограниченного ожидания.

[Генератор](make_movies.py) самодостаточен: stdlib, точные AVM1 bytes, PNG/GIF/JPEG,
[linked input](movie.inc). JPEG — фиксированная цветовая fixture 8×8;
Pillow при запуске не нужен. [AS](png-start.as)/[after AS](after.as) поясняют кадры,
не выдаются за проверку незавершённого backend компилятора.

Команды native configure/build, сервера, трёх форматов, embedded и CTest:
[English](README.md). Цель `flashvm_example_image_loading`; нужны target SDL2 SDK,
C89/C++98, CMake и профиль player-images.defs с приложенными статическими codecs.
Сервер рекомендуется ограничить loopback. Успех: код 0, запросы startup/image/after.
Для --embedded стартовый HTTP-запрос отсутствует, URL явно задаёт базу. Временного
SWF-файла нет. Это быстрый lifecycle-пример; для просмотра медленнее использовать
player --async-loads --loads 8 --frames 500 --fps 30 --mute с тем же URL.

Другая ОС/CPU: [toolchain](../../../doc/DESKTOP_FRONTEND.ru.md) и
[сеть/TLS](../../../doc/SOCKET_TRANSPORT.ru.md). Та же цель, target SDL/codec SDK;
линковка не означает запуск. Curl выбирается через FVM_PLAYER_HTTP вместо
SOCKET_HTTP и требует безопасной global initialization.

Descriptors, URL и userdata живут до синхронного возврата. Worker владеет только
bytes; pin, декодирование и публикация выполняются владельцем. Join предшествует
снятию pin. После import pixels принадлежат графу клипа, до него bytes — job.

| C API | Назначение, владение и блокирование; руководство |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Проверяет worker без I/O; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Инициализация структуры вызывающего; PLAYER_CONFIGURATION.md. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Descriptor провайдера без job; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Native worker, заимствованная policy, cooperative stop; SOCKET_TRANSPORT.md. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Curl worker с проверкой global initialization; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_run_source_async](../../../frontend/common/background.h#L40) | Заимствует descriptors до синхронного возврата; BACKGROUND_TRANSFER.md. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L51) | Завершает SDK после join/cleanup; PLAYER_HOST_OWNERSHIP.md. |

Только static full-canvas GIF89a; GIF87a/animation, root image startup и synthetic
loader/unload events не реализованы. Действуют source profiles и пределы bytes/
размеров. Progressive streaming decode, физическое окно и другая ОС не принимаются
этим тестом. C-хост включает mute; прежние root streaming/audio ограничения остаются.

## Именованные размеры и адрес (v0.706)

Поставляемые SWF читают `_width`/`_height`, удваивают ширину и утраивают высоту
через SetMember, затем сверяют результат с числовыми индексами 8/9. Сохраняют
`_url`, пробуют запись в него, выгружают картинку и загружают SWF относительно
прежнего источника. Встроенный массив пересоздан тем же генератором;
`png-start.as` остаётся подробно прокомментированным эскизом кадров.

| Свойство | Значение и управление |
| --- | --- |
| `_width` | Текущая ширина в координатах родителя; запись преобразует аргумент и меняет масштабы. |
| `_height` | Текущая высота; использует ту же геометрию, что числовой индекс 9. |
| `_url` | Сохранённый источник только для чтения; запись не меняет базу относительной загрузки. |

[Полный контракт](../../../doc/MOVIECLIP_NAMED_PROPERTIES.ru.md) /
[EN](../../../doc/MOVIECLIP_NAMED_PROPERTIES.md). Полный цикл событий загрузчика
и приёмка относительно оригинального плеера пока не заявлены.

Относительные запросы из корневого кадра используют источник вызывающего корня.
Свойство `_url` картинки отдельно описывает её источник, включая конечный URL
после перенаправления.

## Запросы завершённого содержимого (v0.709)

Исполняемые SWF проверяют точные getBytesLoaded/getBytesTotal для каждой картинки,
getSWFVersion=-1, сохранение счётчиков сразу после отложенного запроса выгрузки
в том же кадре и канонические размеры/версию после перезагрузки SWF. Проверяется
и версия корневого SWF. Запуск по URL и из памяти использует один пересозданный
сценарий.

| Метод | Результат в примере |
| --- | --- |
| `getBytesLoaded()` | Полная длина данных картинки, затем распакованного SWF. |
| `getBytesTotal()` | Тот же опубликованный размер; не прогресс worker и не частичная декодировка. |
| `getSWFVersion()` | -1 для картинки, 8 для загруженного SWF; профиль языка хранится отдельно. |

[Полный контракт](../../../doc/MOVIECLIP_CONTENT_INFO.ru.md) /
[EN](../../../doc/MOVIECLIP_CONTENT_INFO.md). Учёт внутренних символов,
прогрессивная загрузка и полная приёмка событий остаются открытыми.
