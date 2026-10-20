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
