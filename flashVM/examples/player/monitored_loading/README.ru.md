# Наблюдение загрузок и отмена старта из C-хоста

[English](README.md) · [Каталог](../../README.md#example-player-monitored_loading) ·
[Контракт API](../../../doc/LOAD_MONITOR.ru.md) · [Кросс-сборка](../../../doc/SOCKET_TRANSPORT.ru.md).

Самостоятельный C89-хост задаёт общий пятисекундный бюджет сетевого задания,
печатает ID/вид/фазу/байты и умеет отменить старт до провайдера и окна. Каждая строка
[main.c](main.c) пояснена. Используются готовые [async SWF, AS и генератор](../async_loading/README.ru.md),
без дублирования ресурсов и зависимостей от тестов или незавершённых компиляторов.

Нужны C89/C++98, CMake и SDL2 SDK целевой платформы. Из корня репозитория:

```sh
cmake -S flashVM -B build-monitor -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-monitor --target flashvm_example_monitored_loading flashvm_player
python3 -m http.server 8000 --directory flashVM/examples/player/async_loading
```

В другом терминале:

```sh
./build-monitor/frontend/flashvm_example_monitored_loading http://127.0.0.1:8000/start.swf
./build-monitor/frontend/flashvm_example_monitored_loading http://127.0.0.1:8000/start.swf --cancel-startup
```

Первый запуск: четыре запроса, успешные проверки кадров/таймеров/значений,
журнал уведомлений, exit 0. Второй: BEGIN/CANCELLED, без HTTP запроса, exit 0.
FETCHED означает успех получения байтов; некорректный SWF ещё может дать ошибку
импорта. Проценты не вычисляются: received — байты тела провайдера, а не wire size
или гарантированный Content-Length. Callback копирует скаляры, строки заимствованы
только на время вызова.

Это короткий loopback-пример: 700 тиков при 1000 FPS. Для обычных фильмов задайте
нормальные FPS/лимиты и собственную политику остановки. Ответ сервера с задержкой
больше бюджета даст TIMED_OUT и playback failure. Отмена/дедлайн кооперативные:
DNS/пользовательский код могут задержать join. Worker не detach; userdata/настройки
провайдера живут до возврата runner.

HTTPS: FVM_PLAYER_SOCKET_TLS, целевой OpenSSL/trust. Curl: выключить SOCKET_HTTP/TLS,
включить FVM_PLAYER_HTTP; старые небезопасные global APIs отклоняются. Static/shared,
SDK/CPU/toolchain/trust описаны в build guide. Cross-link не равен запуску на цели;
Dummy SDL проверяет протокол, не физические устройства.

| API | Ownership / Владение |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Default worker capability, no network/window startup. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller playback scalars. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Initialize caller worker/byte/load policy. |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Initialize async delivery, zero deadline and no callback. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Worker-safe native provider; nested userdata borrowed through return. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Worker-safe curl alternative, requires thread-safe global SDK. |
| [fvm_desktop_run_source_monitored](../../../frontend/common/load_monitor.h#L57) | Whole-player call; notify runs on owner, provider on worker. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L52) | Cleanup only after joined workers/window/audio disposal. |

Та же политика доступна через [CLI/playlist API](../../../doc/LOAD_MONITOR.ru.md#адаптеры-playlist-и-консоли-v0686).
