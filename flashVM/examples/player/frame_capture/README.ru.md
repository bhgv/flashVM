# Захват кадра плеера в память C89-хоста

[English](README.md) · [Каталог](../../README.md#example-player-frame_capture) ·
[Контракт кадров](../../../doc/FRAME_CAPTURE.ru.md).

Самостоятельный [main.c](main.c) показывает callback логического RGBA-растра для
IDE, собственного encoder или копии в другом C/C++-приложении. Он копирует кадр
в native-память, штатно останавливает запуск и пишет PPM только после очистки
VM/окна/загрузок и SDK. В библиотеке нет привязки к файлу или console argv:
аргументы и output adapter принадлежат этому маленькому исполняемому примеру.

[make_movie.py](make_movie.py) кодирует настоящий двухкадровый [SWF8](start.swf) и
идентичные immutable [linked-байты](movie.inc) только через Python stdlib.
Нечётный такт рисует синий квадрат слева, чётный — оранжевый справа; старый рисунок
стирается. [start.as](start.as) — поясняющий AS2-эскиз, не тест AS/FIR-компилятора.
Все непустые строки C/AS/linked-байтов пояснены комментариями.

## Сборка и запуск на своей платформе

Нужны C89/C++98 toolchain, CMake, graphics-enabled профиль FlashVM и SDL2 2.0.9+
SDK своей CPU/OS. JIT, image codecs, Python/Pillow для самого C-примера не нужны.
Локальный/буферный запуск не требует сети. Из корня репозитория:

```sh
cmake -S flashVM -B build-capture -DFVM_BUILD_DESKTOP_FRONTEND=ON \
  -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=ON \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs" \
  -DSDL2_DIR=/path/to/native-sdl/lib/cmake/SDL2
cmake --build build-capture --target flashvm_example_frame_capture \
  flashvm_frontend_frame_copy flashvm_frontend_frame_capture --parallel
./build-capture/frontend/flashvm_example_frame_capture flashVM/examples/player/frame_capture/start.swf first.ppm
./build-capture/frontend/flashvm_example_frame_capture --tick 2 flashVM/examples/player/frame_capture/start.swf second.ppm
./build-capture/frontend/flashvm_example_frame_capture --embedded --tick 3 "$PWD/not-read.swf" embedded.ppm
ctest --test-dir build-capture -R 'frame_capture|frontend_frame_copy' --output-on-failure
python3 flashVM/examples/player/frame_capture/make_movie.py
```

Ожидаются код 0, captured_tick=1/2/3 и width=32 height=32. Изображения — binary
P6 PPM с RGB-каналами: первый/третий синие слева, второй оранжевый справа, фон белый.
Статистика frames равна выбранному host tick. Это advance корня, не инструкция AVM1
или номер кадра вложенного клипа. Значение --tick ограничено 1..1000.
Встроенный режим не читает not-read.swf; абсолютный путь задаёт явный origin/base
для относительных script loads. Нового временного SWF нет.

Пример открывает штатное SDL-окно, которое быстро закрывается после копирования.
Для display-free проверки задайте `SDL_VIDEODRIVER=dummy`; это не нативная приёмка
устройства. Captured pixels — logical stage, без window chrome/DPI/letterbox.
PPM отбрасывает alpha, не выполняя дополнительного compositing; наш фильм имеет
непрозрачный фон. Public copy helper сохраняет все четыре RGBA-канала.

## Сеть и другие платформы

Для HTTP добавьте `-DFVM_PLAYER_SOCKET_HTTP=ON`. HTTPS дополнительно требует
`-DFVM_PLAYER_SOCKET_TLS=ON` и целевых TLS-библиотек/trust. Альтернатива —
`-DFVM_PLAYER_HTTP=ON` с worker-safe libcurl. Без provider URL явно завершается
ошибкой до старта, а локальный/встроенный режим доступен. Бюджет ресурсов — восемь,
на network job задан кооперативный deadline; это не hard kill DNS/provider.

```sh
python3 -m http.server 8000 --bind 127.0.0.1 --directory flashVM/examples/player/frame_capture
# Another terminal / Другой терминал:
./build-capture/frontend/flashvm_example_frame_capture --tick 2 http://127.0.0.1:8000/start.swf network.ppm
./build-capture/frontend/flashvm_example_frame_capture --embedded http://127.0.0.1:8000/not-read.swf linked.ppm
```

Первый запуск делает один startup GET, второй не делает его. У этого фильма нет
дочерних ресурсов; отдельные live-регрессии проверяют смену корня и отмену активного
HTTP при ошибке observer. Подготовка SDK, native/cross и Android surface:
[DESKTOP_FRONTEND.md](../../../doc/DESKTOP_FRONTEND.md),
[SOCKET_TRANSPORT.ru.md](../../../doc/SOCKET_TRANSPORT.ru.md).
Для другой CPU/OS используйте отдельный build-каталог/toolchain и целевой SDL SDK:

```sh
cmake -S flashVM -B build-capture-target \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/target-toolchain.cmake \
  -DSDL2_DIR=/path/to/target-sdl/lib/cmake/SDL2 \
  -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_EXAMPLES=ON -DFVM_BUILD_TESTS=OFF \
  -DFVM_DEFS_FILE="$PWD/flashVM/cmake/profiles/player-jit.defs"
cmake --build build-capture-target --target flashvm_example_frame_capture --parallel
```

Для shared-хоста добавьте `-DFVM_PLAYER_HOST_SHARED=ON`, runtime search paths —
по [правилам SDK](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md). Все SDL/TLS/curl
зависимости должны соответствовать target. Android исключает этот desktop target:
нужен Activity/surface adapter из общего руководства. Кросс-линковка ARM64/RISC-V64/
Windows x64 не означает запуск на этих системах. Отделённому VM-компоненту
достаточно своего -S/profile и явных SDK; соседние компиляторы/линкер не нужны.

## API, время жизни и ошибки

Frame observer не получает VM-указатели. pixels/source/frame заимствованы лишь
внутри callback; independent buffer выделяется/копируется до возврата. Для фонового
consumer хост должен сам синхронизировать свою копию; заимствованный buffer туда
не передаётся. [GC:] Native pixels не являются root/pin и освобождаются явно.
Тот же owner control читает native captured-флаг и возвращает штатный STOP.
После runner возврата очищается SDK, затем encoder использует только свою память.

| C API | Назначение, владение и блокирование |
| --- | --- |
| [fvm_desktop_options_init](../../../frontend/common/options.h#L48) | Заполняет скалярные defaults без старта. [PLAYER_CONFIGURATION.ru.md](../../../doc/PLAYER_CONFIGURATION.ru.md). |
| [fvm_desktop_playback_control_init](../../../frontend/common/playback.h#L68) | Заполняет политику команд владельца без памяти; callback штатно останавливает после копии. [PLAYBACK_CONTROL.ru.md](../../../doc/PLAYBACK_CONTROL.ru.md). |
| [fvm_desktop_frame_observer_init](../../../frontend/common/frame.h#L57) | Заполняет точный размер ABI observer и нули без старта. [FRAME_CAPTURE.ru.md](../../../doc/FRAME_CAPTURE.ru.md). |
| [fvm_desktop_frame_copy](../../../frontend/common/frame.h#L65) | Проверяет оба диапазона до записи видимых RGBA-строк; padding прежний, без памяти. [FRAME_CAPTURE.ru.md](../../../doc/FRAME_CAPTURE.ru.md). |
| [fvm_desktop_background_loader_init](../../../frontend/common/background.h#L21) | Заполняет явную политику provider байтов без workers/I/O. [BACKGROUND_TRANSFER.ru.md](../../../doc/BACKGROUND_TRANSFER.ru.md). |
| [fvm_desktop_load_monitor_init](../../../frontend/common/load_monitor.h#L49) | Заполняет async/deadline наблюдение владельца без заданий. [LOAD_MONITOR.ru.md](../../../doc/LOAD_MONITOR.ru.md). |
| [fvm_desktop_loader_init](../../../frontend/common/source.h#L51) | Заполняет обычный loader; перед запуском явно увеличивается бюджет local-ресурсов. [PLAYER_SOURCES.ru.md](../../../doc/PLAYER_SOURCES.ru.md). |
| [fvm_desktop_socket_transfer_fetch](../../../frontend/common/socket_http.h#L65) | Выбранный native worker HTTP/TLS provider с кооперативной отменой; без pixels/VM. [SOCKET_TRANSPORT.ru.md](../../../doc/SOCKET_TRANSPORT.ru.md). |
| [fvm_desktop_http_transfer_fetch](../../../frontend/common/http.h#L35) | Альтернативный worker-safe curl provider с проверкой SDK globals. [BACKGROUND_TRANSFER.ru.md](../../../doc/BACKGROUND_TRANSFER.ru.md). |
| [fvm_desktop_run_source_observed](../../../frontend/common/frame.h#L76) | Синхронный local/memory/URL запуск с необязательными loader/control и обязательным observer; borrow до возврата, код 0/1/2. [FRAME_CAPTURE.ru.md](../../../doc/FRAME_CAPTURE.ru.md). |
| [fvm_desktop_run_source_background_observed](../../../frontend/common/frame.h#L83) | Требует provider/monitor и observer; callbacks только владельцу, join до освобождения buffer/roots. [FRAME_CAPTURE.ru.md](../../../doc/FRAME_CAPTURE.ru.md). |
| [fvm_desktop_shutdown](../../../frontend/common/player.h#L54) | Явное завершение сессии после ресурсов runner; копия остаётся native-памятью хоста. [PLAYER_HOST_OWNERSHIP.ru.md](../../../doc/PLAYER_HOST_OWNERSHIP.ru.md). |

Аргументы возвращают код 2; source/render/observer/encoding/I/O-ошибка — 1;
успешная копия и полная запись — 0. При неверном источнике output не открывается.
Если пользователь закрыл окно до нужного tick, изображение не создаётся и код 1.
Ошибки callback не откатывают уже выполненную VM/показ; SDK дожидается pending jobs.
Ошибка записи может оставить частичный PPM; это обычная политика примера, библиотека
не пишет файл. Output filename подчиняется native C runtime. Файл закрывается,
копия освобождается на всех путях.

Границы: SDL window остаётся частью плеера; это не widget/windowless API.
Нет доступа к чужим SDL events, GPU screenshot, audio/video encoder или полного
отладчика. Нет reentrant/concurrent runner; VM/SDL принадлежат UI owner. Captured
RGBA не обещает полную Flash/SWF8/эталонную совместимость. Native foreign OS/device
приёмка и все стадии, включая пункт 9, остаются открытыми.
