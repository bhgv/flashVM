# Фоновый провайдер загрузки из C

[English](README.md) · [Каталог](../../README.md#example-player-background_loading) ·
[Контракты](../../../doc/BACKGROUND_TRANSFER.ru.md) · [Источники](../../../doc/PLAYER_SOURCES.ru.md).

Исполняемый C89-пример выбирает worker-safe callback для конкретного запуска.
Старт, child и формы загружаются через POSIX/Winsock либо необязательный libcurl.
Основной поток продолжает обслуживать активное native-окно, пока приходят байты.
Timeline и script timers ждут на ресурсной безопасной границе: это не полная
асинхронная семантика Flash. При startup/полной root-замене активного view нет;
для собственного progress UI хоста предназначен нижний transfer API.

Нужны C89/C++98-компиляторы, CMake и целевой SDL2 SDK. Сокетам libcurl не нужен.
Из корня репозитория:

```sh
cmake -S flashVM -B build-background -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-background --target flashvm_player flashvm_example_background_loading
python3 -m http.server 8000 --directory flashVM/examples/player/loading
```

В другом терминале:

```sh
./build-background/frontend/flashvm_example_background_loading http://127.0.0.1:8000/start.swf
./build-background/frontend/flashvm_player --background-loads --loads 8 --frames 3 --mute http://127.0.0.1:8000/start.swf
```

Ожидается: child и формы загружены, проверки SWF проходят, status 0. Закрытие во
время дочернего fetch отменяет/присоединяет worker и даёт 0 без импорта частичного
SWF. DNS/некооперативный callback могут задержать join. Нет detached workers и
записи файлов. Dummy SDL допустим только как headless protocol test.

Для HTTPS включите FVM_PLAYER_SOCKET_TLS с целевым OpenSSL SDK. Trust — defaults
OpenSSL, включая SSL_CERT_FILE, либо явные native options. Для libcurl отключите
SOCKET_HTTP/TLS и включите FVM_PLAYER_HTTP; нужен linked SDK с CURL_VERSION_THREADSAFE.
Static SDK и cross-build OS/CPU описаны в руководстве транспорта; link не равен run.

[fvm_desktop_background_supported](../../../frontend/common/background.h#L58)() проверяет штатную worker-поддержку до старта.
Асинхронный вариант: [пример](../async_loading/README.ru.md).

В [main.c](main.c) объяснена каждая строка. Options, URL и loader заимствованы до
возврата; вложенный userdata immutable/thread-safe. Worker накапливает native
bytes; импорт/setters выполняет owner. VM-объекты не передаются callback. Все
задания присоединяются до SDK shutdown.

| API | Назначение, владение и блокирование |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L53) | Инициализация scalar options вызывающей программы без startup. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L22) | Инициализация caller policy без запуска worker. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L86) | Native worker provider с borrowed options/cancel; DNS может блокировать. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L52) | Worker adapter libcurl; SDK без thread-safe global APIs отклоняется. |
| [fvm_desktop_run_source_background](../../../frontend/common/background.h#L31) | Синхронный owner loop с native events во время ожидания; импорт только готовых байтов. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L65) | Завершение соответствующего SDK после join/очистки окна; живые jobs запрещают его. |

[Контракт C API](../../../doc/BACKGROUND_TRANSFER.ru.md) описывает аргументы/ошибки/
время жизни и нижние begin/poll/cancel/release для IDE с собственным event loop.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_desktop_background_loader_init` | function | [frontend/common/background.h:22](../../../frontend/common/background.h#L22) |
| `fvm_desktop_background_supported` | function | [frontend/common/background.h:58](../../../frontend/common/background.h#L58) |
| `fvm_desktop_http_transfer_fetch` | function | [frontend/common/http.h:52](../../../frontend/common/http.h#L52) |
| `fvm_desktop_options_init` | function | [frontend/common/options.h:53](../../../frontend/common/options.h#L53) |
| `fvm_desktop_run_source_background` | function | [frontend/common/background.h:31](../../../frontend/common/background.h#L31) |
| `fvm_desktop_shutdown` | function | [frontend/common/player.h:65](../../../frontend/common/player.h#L65) |
| `fvm_desktop_socket_transfer_fetch` | function | [frontend/common/socket_http.h:86](../../../frontend/common/socket_http.h#L86) |
<!-- source-links:end -->
