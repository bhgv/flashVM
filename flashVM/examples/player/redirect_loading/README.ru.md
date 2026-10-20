# Redirected SWF и конечные базы адресов фильмов

[English](README.md) · [Каталог](../../README.md#example-player-redirect_loading) ·
[Контракт HTTP](../../../doc/HTTP_REDIRECTS.ru.md) · [Сборка/SDK/CPU](../../../doc/SOCKET_TRANSPORT.ru.md).

[C89-хост](main.c) вызывает структурированный network-policy runner без парсинга
player CLI. Каждая строка C пояснена. [Самостоятельный сервер](serve.py) использует
только Python standard library и готовые ресурсы [async-примера](../async_loading/README.ru.md),
где находятся комментированные AS и генератор. Tests/незавершённый AS/FIR backend
не импортируются.

Startup URL переходит в `/movies/main/start.swf`, относительный child — в
`/sprites/child.swf`. Root form должен разрешиться под `/movies/main/`, child form —
под `/sprites/`; обе формы тоже перенаправляются. Четыре логических ресурса дают
восемь HTTP запросов. Ролик проверяет значения и реальные кадры/таймер; неверные базы
дают ошибку. Промежуточные тела redirects не импортируются.

Нужны C89/C++98, CMake, целевой SDL2 SDK и Python3. Из корня репозитория:

```sh
cmake -S flashVM -B build-redirect -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-redirect --target flashvm_player flashvm_example_redirect_loading
python3 flashVM/examples/player/redirect_loading/serve.py 8000
```

В другом терминале:

```sh
./build-redirect/frontend/flashvm_example_redirect_loading http://127.0.0.1:8000/start.swf
./build-redirect/frontend/flashvm_example_redirect_loading http://127.0.0.1:8000/start.swf --cancel-startup
./build-redirect/frontend/flashvm_player --async-loads --redirects 3 --loads 4 --load-progress --frames 700 --fps 1000 --mute http://127.0.0.1:8000/start.swf
```

Обычный запуск: пустая assertion stage, восемь запросов, четыре load ID, exit 0.
Stop-пример: BEGIN/CANCELLED без HTTP запросов и player view, exit 0. max_loads=4
и per-resource max_redirects=3 независимы. Короткий loopback-пример использует
700 тиков/1000FPS; для фильмов выберите обычный FPS и свою stop/frame policy.
Сервер слушает только loopback, Ctrl-C завершает его.

Дескрипторы, ca_file и observer userdata неизменны/заимствованы до возврата runner.
Observer работает на owner, сеть — на joined worker. DNS/несотрудничающий callback
могут задержать join. FETCHED означает полный fetch до SWF import; ошибки остаются
ошибками playback. Проценты и total-size не обещаются.

HTTPS требует native TLS и целевого OpenSSL/trust SDK. Curl-сборка: отключить
SOCKET_HTTP/TLS, включить FVM_PLAYER_HTTP с thread-safe linked SDK. В C-хосте задайте
policy.ca_file для явного PEM trust. Static/shared SDK, target CPU/toolchain и trust
описаны в build guide. Cross-link не равен foreign execution; dummy SDL проверяет
протокол, не физические устройства.

| API | Purpose / Назначение |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Default worker capability, no network/window startup. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Initialize caller playback settings. |
| [fvm_desktop_network_policy_init](../../../frontend/common/network_loader.h#L13) | Initialize redirect/deadline/trust policy, including provider-less SDKs. |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Async delivery and caller-owned observer policy. |
| [fvm_desktop_run_source_network](../../../frontend/common/network_loader.h#L22) | Whole-player source call; default provider honors HTTP policy on every logical resource. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L53) | Cleanup after workers join and owner disposes the active view. |

Для внешнего HTTPS сервера пример принимает явный PEM через FLASHVM_CA_FILE:
это настройка example host, не глобальная настройка SDK.

```sh
FLASHVM_CA_FILE=/path/to/ca.pem ./build-redirect/frontend/flashvm_example_redirect_loading https://example.test/start.swf
```

CLI использует --http-ca-file PATH, C/IDE — policy.ca_file. Локальный serve.py намеренно
plain HTTP, TLS termination в нём нет. Trust settings неизменны/заимствованы до возврата;
каждый новый redirect destination проверяет сертификат заново.
