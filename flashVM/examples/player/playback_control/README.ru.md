# Пауза и шаги плеера из C89

[English](README.md) · [Каталог](../../README.md#example-player-playback_control) ·
[Контракт управления](../../../doc/PLAYBACK_CONTROL.ru.md).

Самостоятельный [main.c](main.c) показывает C-интерфейс для IDE/хоста: начальную
паузу, шаг, короткое ожидание без блокирующего sleep, ещё один шаг, продолжение и
STOP после пяти тактов. `--step N` вместо этого выполняет ровно N отдельных шагов.
Каждая непустая строка C/байтов/AS пояснена. Это управление шкалой, не отладчик
инструкций. Тонкий CLI лишь заполняет дескрипторы; библиотека не разбирает argv.

[make_movie.py](make_movie.py) использует только Python stdlib и независимо
кодирует настоящий двухкадровый [SWF8](start.swf): синий квадрат слева, оранжевый
справа. [movie.inc](movie.inc) содержит те же неизменяемые байты для линковки.
[start.as](start.as) — поясняющий AS2-эскиз, не проверка компилятора AS/FIR.

## Сборка и запуск

Нужны C89/C++98 toolchain, CMake, графический профиль FlashVM и SDL2 2.0.9+
для целевой платформы. Локальный/встроенный режимы не требуют сети, image codecs
или JIT. Из корня репозитория:

```sh
cmake -S flashVM -B build-control -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-control --target flashvm_example_playback_control --parallel
./build-control/frontend/flashvm_example_playback_control flashVM/examples/player/playback_control/start.swf
./build-control/frontend/flashvm_example_playback_control --step 3 flashVM/examples/player/playback_control/start.swf
./build-control/frontend/flashvm_example_playback_control --embedded --step 7 "$PWD/not-read.swf"
python3 flashVM/examples/player/playback_control/make_movie.py
ctest --test-dir build-control -R 'playback_control|playback_policy' --output-on-failure
```

Ожидаются код 0 и `completed=5`, `completed=3`, `completed=7` соответственно.
Статистика stderr содержит то же число frames и настоящие interpreter-счётчики.
Встроенный режим не читает not-read.swf: абсолютный путь задаёт provenance/base
для возможных относительных запросов. Не передавайте относительный путь как
явную подмену платформенного каталога исполняемого файла.

Для HTTP добавьте при configure `-DFVM_PLAYER_SOCKET_HTTP=ON`; HTTPS требует
`-DFVM_PLAYER_SOCKET_TLS=ON` и целевых TLS-библиотек/trust. Альтернатива —
`-DFVM_PLAYER_HTTP=ON` с worker-safe libcurl. Provider выбирается при сборке.
Без provider локальный/встроенный режим работает, а URL сообщает ошибку.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/playback_control
# Another terminal / Другой терминал:
./build-control/frontend/flashvm_example_playback_control --step 4 http://127.0.0.1:8000/start.swf
./build-control/frontend/flashvm_example_playback_control --embedded --step 4 http://127.0.0.1:8000/start.swf
```

Первый запуск запрашивает /start.swf, второй не запрашивает стартовый фильм.
У самого примера нет дочерних сетевых запросов; общие регрессии управления
отдельно проверяют их публикацию/отмену при паузе. Для display-free проверки
задайте `SDL_VIDEODRIVER=dummy SDL_RENDER_DRIVER=software`; это не аппаратная приёмка.

## Другие платформы и процессоры

Используйте отдельный каталог, CMake toolchain и SDL SDK именно целевого CPU/OS.
Библиотеки TLS/curl и их транзитивные зависимости тоже должны быть целевыми.
Инструкции подготовки SDK, Windows/Linux/macOS и Android находятся в
[DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md), а варианты статической/
динамической линковки — в [PLAYER_HOST_OWNERSHIP.md](../../../doc/PLAYER_HOST_OWNERSHIP.md).

```sh
cmake -S flashVM -B build-control-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-control-target --target flashvm_example_playback_control --parallel
```

Этот desktop target исключён на Android: там нужен Activity/surface adapter,
описанный в общем руководстве. Кросс-линковка ARM64/RISC-V64/Windows не означает
запуск на этих системах. Для отделённого компонента замените `-S flashVM` на
путь к корню компонента и задайте его profile явно; соседние проекты не требуются.

## API и владение

Все функции ниже описаны в указанных руководствах; init-функции возвращают 1/0
для обычного/null указателя, не выделяют память и не запускают native службы.

| C API | Назначение, время жизни и блокирование; руководство |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Заполняет скалярные настройки вызывающего без I/O. [PLAYER_CONFIGURATION.ru.md](../../../doc/PLAYER_CONFIGURATION.ru.md). |
| [fvm_desktop_playback_control_init](../../../frontend/common/playback.h#L68) | Заполняет размер ABI управления, режим работы и нулевые callback/userdata без I/O. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Заполняет заимствованную политику provider без запуска workers. [BACKGROUND_TRANSFER.ru.md](../../../doc/BACKGROUND_TRANSFER.ru.md). |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Заполняет defaults асинхронной доставки/срока без создания заданий. [LOAD_MONITOR.ru.md](../../../doc/LOAD_MONITOR.ru.md). |
| [fvm_desktop_run_source_controlled](../../../frontend/common/playback.h#L77) | Синхронный запуск с необязательным loader и обязательным control; заимствует память до возврата, код 0/1/2. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_run_source_background_controlled](../../../frontend/common/playback.h#L86) | Синхронно владеет запуском; требует provider/monitor/control; дожидается workers до возврата, код 0/1/2. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L54) | Освобождает SDK после завершения окон/заданий запуска; временем жизни процесса управляет вызывающий. [PLAYER_HOST_OWNERSHIP.ru.md](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md). |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Выбранный нативный worker adapter: кооперативно получает неизменяемые байты, без VM/SDL. [SOCKET_TRANSPORT.ru.md](../../../doc/SOCKET_TRANSPORT.ru.md). |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Альтернативный curl worker adapter: проверяет потоковую безопасность SDK перед загрузкой. [BACKGROUND_TRANSFER.ru.md](../../../doc/BACKGROUND_TRANSFER.ru.md). |

Callback observe заимствует снимок только до возврата и сохраняет лишь скаляры.
Он возвращает ровно одну команду; KEEP не снимает паузу. [GC:] Корень закрепляет
runner; не снимайте его pin и не сохраняйте context/root. Копирование указателя
или новый pin не продлевают жизнь контекста запуска. Дескрипторы, argv/буфер и
userdata живут до синхронного возврата; workers к VM не обращаются. STOP завершает
весь запуск и дожидается отменённых заданий до очистки. error — заимствованный
текст; статус 2 означает аргументы, 1 — runtime/provider, 0 — штатное завершение.

Ограничения: владелец native UI-потока, без повторного/одновременного входа;
нет готового GUI IDE или очереди команд из других потоков. Таймеры/ввод/загрузки
могут менять VM при паузе. Начальная загрузка/блокирующее ожидание не вызывает
observe: отмена там задаётся monitor. Пример без звука; PCM-пауза проверяется
отдельными dummy-audio fixtures. Полная SWF8/эталонная и аппаратная приёмка всех
ОС остаётся открытой. Это продвижение плана, не завершение пункта 9.
