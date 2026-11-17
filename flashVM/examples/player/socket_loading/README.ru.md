# Загрузка SWF через встроенные сокеты

[English](README.md) · [Каталог](../../README.md#example-player-socket_loading) ·
[Контракт транспорта](../../../doc/SOCKET_TRANSPORT.ru.md) · [Источники SWF](../../../doc/PLAYER_SOURCES.ru.md).

Исполняемый C89-хост явно выбирает HTTP callback на POSIX/Winsock. Он запускает
сетевой SWF и разрешает загрузку дочерних фильмов и переменных из самого SWF.
Пример подходит для IDE, обычной C-программы и потребителя библиотеки плеера без
зависимости от libcurl. Выполнение синхронное, на основном UI/VM-потоке.

Нужны C89/C++98-компиляторы, CMake, SDL2 SDK целевой платформы и профиль рендера.
Из корня репозитория с установленным SDL2:

```sh
cmake -S flashVM -B build-socket -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_PLAYER_HTTP=OFF -DFVM_PLAYER_SOCKET_HTTP=ON
cmake --build build-socket --target flashvm_example_socket_loading
python3 -m http.server 8000 --directory flashVM/examples/player/loading
```

В другом терминале:

```sh
./build-socket/frontend/flashvm_example_socket_loading http://127.0.0.1:8000/start.swf
```

Ожидается: стартовый фильм импортирует `nested/child.swf`, загружает формы root и
child, проверяет значения; хост заканчивает три такта со статусом 0. Сервер
показывает запросы старта, child и обеих форм. Ошибка сети/импорта даёт ненулевой
код; записи файлов нет. На безэкранном тестовом хосте можно задать
SDL_VIDEODRIVER=dummy и SDL_RENDER_DRIVER=software; это не проверка настоящего окна.

В [main.c](main.c) объяснена каждая строка. Дескрипторы и настройки лежат на стеке
основного потока до возврата runner. Транспорт закрывает сокет при успехе и ошибке;
runner освобождает временные байты и GC pins. Затем вызывающий код завершает
сеанс именно связанной с ним библиотеки плеера.

| API | Назначение, владение и блокирование |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L53) | Инициализация настроек вызывающей программы без открытия display/аллокаций. |
| [fvm_desktop_loader_init](../../../frontend/common/source.h#L64) | Начальная callback-политика без транспорта, с одним разрешённым запросом. |
| [fvm_desktop_socket_options_init](../../../frontend/common/socket_http.h#L38) | Начальные тайм-ауты в структуре вызывающей программы; сокеты не открываются. |
| [fvm_desktop_socket_fetch](../../../frontend/common/socket_http.h#L47) | Синхронные DNS/connect/чтение через заимствованный sink; все собственные сокеты закрываются, указатели не сохраняются. |
| [fvm_desktop_run_source](../../../frontend/common/source.h#L75) | Работа UI/VM-потока с заимствованными дескрипторами до возврата; запросы на безопасных границах. |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L65) | Завершение сеанса библиотеки после плеера; активные SDL-подсистемы запрещают очистку. |

Подробные аргументы/коды/время жизни каждого API — в [контракте транспорта](../../../doc/SOCKET_TRANSPORT.ru.md#c-api)
и [руководстве источников](../../../doc/PLAYER_SOURCES.ru.md). Пример задаёт 2 с
на TCP connect, 5 с на весь обмен одного ресурса и восемь загрузок. Системный DNS
синхронный: эти тайм-ауты не могут прервать сам resolver. Поддерживается строгая
часть HTTP/1.x без redirects, proxy, POST и автоматической отправки переменных.
HTTPS требует отдельной сборки с проверяемым TLS. Для cross-build нужен SDL SDK
целевой платформы; инструкция в руководстве транспорта. Проверка запуска на
другой ОС/CPU проводится отдельно.

Для проверяемого HTTPS включите FVM_PLAYER_SOCKET_TLS и SDK OpenSSL. Второй необязательный аргумент задаёт PEM CA; без него используется default trust OpenSSL. См. [TLS-контракт](../../../doc/SOCKET_TRANSPORT.ru.md#проверяемый-https-с-v0678).

IDE может заранее вызвать [fvm_desktop_network_capabilities](../../../frontend/common/network.h#L22) из [network.h](../../../frontend/common/network.h), чтобы узнать default provider без запуска сети/display. Сам пример явно выбирает свой callback.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_desktop_loader_init` | function | [frontend/common/source.h:64](../../../frontend/common/source.h#L64) |
| `fvm_desktop_network_capabilities` | function | [frontend/common/network.h:22](../../../frontend/common/network.h#L22) |
| `fvm_desktop_options_init` | function | [frontend/common/options.h:53](../../../frontend/common/options.h#L53) |
| `fvm_desktop_run_source` | function | [frontend/common/source.h:75](../../../frontend/common/source.h#L75) |
| `fvm_desktop_shutdown` | function | [frontend/common/player.h:65](../../../frontend/common/player.h#L65) |
| `fvm_desktop_socket_fetch` | function | [frontend/common/socket_http.h:47](../../../frontend/common/socket_http.h#L47) |
| `fvm_desktop_socket_options_init` | function | [frontend/common/socket_http.h:38](../../../frontend/common/socket_http.h#L38) |
<!-- source-links:end -->
