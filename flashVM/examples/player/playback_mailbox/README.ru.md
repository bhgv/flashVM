# Контроллер плеера в отдельном потоке C89

[English](README.md) · [Каталог](../../README.md#example-player-playback_mailbox) ·
[Контракт ящика](../../../doc/PLAYBACK_MAILBOX.ru.md).

Исполняемый [main.c](main.c) — образец для IDE/нативного C/C++-хоста. Главный
UI-поток запускает плеер, отдельный OS-поток передаёт только native-команды и
читает скопированные скаляры/текст. По умолчанию выполняются пять подтверждённых
шагов, затем STOP; `--steps N` задаёт 1..1000 шагов. Перед следующим STEP контроллер
ждёт завершения предыдущего. Быстрые повторные STEP объединяются и не являются
счётчиком команд. Библиотека не разбирает argv: это делает только тонкий main.

[make_movie.py](make_movie.py) независимо кодирует настоящий двухкадровый
[start.swf](start.swf) и тот же immutable [movie.inc](movie.inc), используя только
Python stdlib. На кадрах синий квадрат слева сменяется оранжевым справа.
[start.as](start.as) — поясняющий AS2-эскиз, не проверка AS/FIR-компилятора.
Все непустые строки C/AS/linked-байтов прокомментированы.

## Требования, сборка и запуск

C89/C++98 toolchain, CMake, graphics-enabled профиль FlashVM, SDL2 2.0.9+ SDK для
целевой CPU/OS. JIT, image codecs и сеть для локального/встроенного режима не нужны.
В примере SDL — сменяемый адаптер OS-потоков/atomic/часов; публичный mailbox ABI
не раскрывает SDL-типы. Собственный хост может использовать pthread/Win32/другой
механизм потоков при соблюдении тех же правил владения.

```sh
cmake -S flashVM -B build-mailbox -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-mailbox --target flashvm_example_playback_mailbox --parallel
./build-mailbox/frontend/flashvm_example_playback_mailbox flashVM/examples/player/playback_mailbox/start.swf
./build-mailbox/frontend/flashvm_example_playback_mailbox --steps 7 flashVM/examples/player/playback_mailbox/start.swf
./build-mailbox/frontend/flashvm_example_playback_mailbox --embedded --steps 3 "$PWD/not-read.swf"
ctest --test-dir build-mailbox -R playback_mailbox --output-on-failure
python3 flashVM/examples/player/playback_mailbox/make_movie.py
```

Без закрытия окна пользователем ожидаются exit 0 и `completed=5`, `completed=7`,
`completed=3`. Статистика frames совпадает, interpreter-счётчики ненулевые.
Встроенный режим не читает not-read.swf: абсолютный путь задаёт явный origin/base.
Он не создаёт временный SWF. После release печатается copied_source из собственной
копии текста. При закрытии окна раньше показано меньшее число завершённых тактов.
Для display-free протокольной проверки задайте `SDL_VIDEODRIVER=dummy`; это не
аппаратная приёмка экрана или ОС.

Для HTTP добавьте `-DFVM_PLAYER_SOCKET_HTTP=ON` при configure. HTTPS дополнительно
требует `-DFVM_PLAYER_SOCKET_TLS=ON` и целевых TLS-библиотек/trust. Альтернатива —
`-DFVM_PLAYER_HTTP=ON` с worker-safe libcurl. Без provider URL явно завершается
ошибкой, а локальный/встроенный запуск доступен. Monitor получает тот же ящик, что
и playback callback, поэтому STOP виден также до READY и при блокирующем fetch.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/playback_mailbox
# Another terminal / Другой терминал:
./build-mailbox/frontend/flashvm_example_playback_mailbox --steps 4 http://127.0.0.1:8000/start.swf
./build-mailbox/frontend/flashvm_example_playback_mailbox --embedded --steps 4 http://127.0.0.1:8000/not-read.swf
```

Первый вариант делает один startup GET; второй не делает его. В фильме нет
дочерних запросов; независимые live-регрессии проверяют отмену startup/child после
начала тела ответа и доставку loadVariables на паузе. При ошибке источника ещё
до READY owner выставляет done, дожидается контроллера, освобождает mailbox и SDK;
worker не остаётся ждать несуществующий снимок.

## Другая платформа или процессор

Отдельный build-каталог, CMake toolchain и SDL SDK целевого CPU/OS обязательны.
TLS/curl и транзитивные зависимости тоже должны быть целевыми. Полная подготовка
SDK/платформ: [DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md), политика
[SOCKET_TRANSPORT.ru.md](../../../doc/SOCKET_TRANSPORT.ru.md).

```sh
cmake -S flashVM -B build-mailbox-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-mailbox-target --target flashvm_example_playback_mailbox --parallel
```

Android исключает этот desktop target: нужен Activity/surface adapter из общего
руководства. Кросс-линковка ARM64/RISC-V64/Windows x64 не доказывает запуск на этих
системах. Для отделённого VM-компонента укажите его корень в -S и его profile;
соседние компиляторы/линкер не требуются. Native SDL зависимости target наследует
от flashvm_player_host; статическую/динамическую модель выбирает SDK-конфигурация.

Для shared-хоста добавьте `-DFVM_PLAYER_HOST_SHARED=ON` при configure; runtime
DLL/shared-library search path задавайте по [правилам SDK](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md).

## API и владение

Каждая строка ниже относится к своей функции; ссылки ведут к полному контракту.

| C API | Назначение, владение и блокирование |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Заполняет скалярные defaults вызывающего без выделения памяти/старта. [PLAYER_CONFIGURATION.ru.md](../../../doc/PLAYER_CONFIGURATION.ru.md). |
| [fvm_desktop_playback_control_init](../../../frontend/common/playback.h#L68) | Заполняет дескриптор команд владельца и точный размер ABI без старта. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_playback_snapshot_init](../../../frontend/common/playback_mailbox.h#L32) | Заполняет размер копии/нули без получения ресурсов. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_playback_mailbox_create](../../../frontend/common/playback_mailbox.h#L38) | Создаёт native mutex/ограниченную запись; при ошибке out прежний, без VM/потока. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_playback_mailbox_post](../../../frontend/common/playback_mailbox.h#L47) | Потокобезопасное последнее намерение; принятый STEP не означает завершённый такт, STOP окончателен. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_playback_mailbox_poll](../../../frontend/common/playback_mailbox.h#L52) | Короткая копия под mutex; NOT_FOUND до READY, без доступа к VM. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_playback_mailbox_release](../../../frontend/common/playback_mailbox.h#L42) | Освобождает native запись после runner/контроллера, не дожидается их самостоятельно. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_playback_mailbox_notify](../../../frontend/common/playback_mailbox.h#L62) | Callback владельца копирует наблюдение/потребляет намерение, не сохраняет VM-указатель. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_playback_mailbox_load_notify](../../../frontend/common/playback_mailbox.h#L66) | Load callback владельца читает непотребляемую отмену до/при ожидании fetch. [PLAYBACK_MAILBOX.ru.md](../../../doc/PLAYBACK_MAILBOX.ru.md). |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Заполняет provider policy без workers/I/O. [BACKGROUND_TRANSFER.ru.md](../../../doc/BACKGROUND_TRANSFER.ru.md). |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Заполняет async/deadline defaults наблюдения без заданий. [LOAD_MONITOR.ru.md](../../../doc/LOAD_MONITOR.ru.md). |
| [fvm_desktop_run_source_background_controlled](../../../frontend/common/playback.h#L86) | Синхронно заимствует provider/monitor/control/source до joined-очистки владельца, статус 0/1/2. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_run_source_controlled](../../../frontend/common/playback.h#L77) | Локальный/буферный управляемый запуск с необязательным loader; вход заимствован до возврата, статус 0/1/2. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Настроенный native worker provider байтов; кооперативная отмена, без VM/SDL-событий. [SOCKET_TRANSPORT.ru.md](../../../doc/SOCKET_TRANSPORT.ru.md). |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Альтернативный worker-safe curl provider с проверкой SDK globals. [BACKGROUND_TRANSFER.ru.md](../../../doc/BACKGROUND_TRANSFER.ru.md). |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L53) | Явное завершение SDK после всех native mailbox/worker/окон/заданий. [PLAYER_HOST_OWNERSHIP.ru.md](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md). |

Native-адаптер примера использует: [SDL_CreateThread](https://wiki.libsdl.org/SDL2/SDL_CreateThread), [SDL_WaitThread](https://wiki.libsdl.org/SDL2/SDL_WaitThread), [SDL_AtomicGet](https://wiki.libsdl.org/SDL2/SDL_AtomicGet), [SDL_AtomicSet](https://wiki.libsdl.org/SDL2/SDL_AtomicSet), [SDL_GetTicks](https://wiki.libsdl.org/SDL2/SDL_GetTicks), [SDL_Delay](https://wiki.libsdl.org/SDL2/SDL_Delay).

GetTicks инициализируется на main до worker; короткие интервалы проверяются через
беззнаковую разность Uint32. WaitThread вызывается ровно один раз, его handle
после join больше не используется. После возврата runner owner выставляет done
даже при ошибке до READY. Затем join → snapshot copy → mailbox release → shutdown.
SDK не завершится, пока жив ящик. Snapshot source — своя копия, не borrowed VM-текст.

Ограничения: C API работает на owner UI-потоке, без одновременных/повторных запусков.
Workers не входят в VM/GC, не рисуют и не забирают UI events. Пауза шкалы не
замораживает таймеры/сеть/явные скриптовые переходы. STOP кооперативен, не прерывает
native callback/DNS. Idle-ошибка контроллера после пяти секунд без прогресса
возвращает код 1, не обещая hard deadline для provider join. Пример без звука;
нет GUI IDE, отладки инструкций или доказательства полной Flash/SWF8-совместимости.
Нативная/аппаратная приёмка остальных ОС и пункт 9 остаются открытыми.
