# Асинхронные дочерние фильмы и переменные форм

[English](README.md) · [Каталог](../../README.md#example-player-async_loading) ·
[Контракты](../../../doc/BACKGROUND_TRANSFER.ru.md) · [Источники](../../../doc/PLAYER_SOURCES.ru.md).

C89-хост выбирает [fvm_desktop_run_source_async](../../../frontend/common/background.h#L40). Сетевые child/form запросы идут
по FIFO, один worker за раз, пока родительские кадры, таймеры, ввод и GC safe points
продолжаются. Готовые байты импортируются/декодируются только owner. Startup и
полная root-замена сохраняют background lifecycle: это ограниченная async loading,
не полная совместимость всех Flash loaders.

Пример самодостаточен: [C](main.c), пояснённые [root AS](start.as) и
[child AS](nested/child.as), SWF и обе формы. [Генератор](make_movies.py) использует
только Python standard library, выдаёт детерминированные AVM1 bytes без импорта
тестов или незавершённого AS/FIR backend. AS-файлы — timeline sketches, не приёмка
parser/backend. Пример допускает быстрый ответ до второго кадра. Отдельный
slow-server regression требует pending запрос во втором кадре, отличая async
исполнение от режима ожидания.

Нужны C89/C++98-компиляторы, CMake и SDL2 SDK цели. Из корня репозитория:

```sh
cmake -S flashVM -B build-async -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON -DFVM_PLAYER_SOCKET_HTTP=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-async --target flashvm_player flashvm_example_async_loading
python3 flashVM/examples/player/async_loading/make_movies.py
python3 -m http.server 8000 --directory flashVM/examples/player/async_loading
```

В другом терминале:

```sh
./build-async/frontend/flashvm_example_async_loading http://127.0.0.1:8000/start.swf
./build-async/frontend/flashvm_player --async-loads --loads 8 --frames 700 --fps 1000 --mute http://127.0.0.1:8000/start.swf
```

Ожидается: обычная пустая сцена, четыре HTTP запроса (startup, child, root vars,
child vars), успешные внутренние frame/timer/value assertions, exit 0. Это быстрый
ограниченный loopback-пример, не анимация или Internet latency benchmark. Два
wait-кадра не повторяют initialization; root ждёт оба origin и настоящий timer,
удаляет timer и останавливается. Неверные/отсутствующие значения дают wait assertion.
Frame limit/close/error всё равно join незавершённую работу. Для настоящего фильма
выберите нормальный FPS и свою политику завершения. Dummy SDL не заменяет
физическое окно/foreign OS acceptance.

HTTPS требует FVM_PLAYER_SOCKET_TLS, целевой OpenSSL SDK и trust. Для libcurl
отключите SOCKET_HTTP/TLS, включите FVM_PLAYER_HTTP; linked SDK должен сообщать
thread-safe global APIs. Пример сообщает отсутствие worker capability до startup.
Static SDK/cross-toolchain/CPU/trust описаны в руководстве транспорта. Глобального
mutable setter провайдера нет.

| API | Назначение, владение и блокирование |
| --- | --- |
| [fvm_desktop_background_supported](../../../frontend/common/background.h#L49) | Проверка default worker capability без network/display; custom callback отдельно. |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Инициализация scalar options вызывающей программы. |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Инициализация caller policy без worker. |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L64) | Native worker adapter с immutable borrowed options/cancel. |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L34) | Curl worker adapter, отклоняющий старый unsafe global init. |
| fvm_desktop_run_source_async | Синхронный whole-player call с неблокирующей доставкой child/form на owner safe points. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L50) | Завершение SDK после joined jobs и очистки window/audio. |

Стековые дескрипторы и вложенный userdata живут неизменными до возврата. Worker
не получает VM-объектов. Target удерживается pin до доставки либо joined cancel.
Job хранит байты до окончания owner import, затем release освобождает их. DNS
может задержать join; worker не убивается и не detach. Сохраняются строгие player
ошибки; отсутствующие MovieClipLoader progress/failure events не имитируются.
