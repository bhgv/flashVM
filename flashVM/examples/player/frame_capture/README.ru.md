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
  flashvm_frontend_frame_copy flashvm_frontend_frame_capture \
  flashvm_frontend_offscreen_window flashvm_frontend_offscreen_capture --parallel
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

Без --offscreen пример открывает штатное SDL-окно, которое быстро закрывается после копирования.
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

## Windowless / Без собственного окна (v0.721)

```sh
./build-capture/frontend/flashvm_example_frame_capture --offscreen --tick 2 flashVM/examples/player/frame_capture/start.swf offscreen.ppm
./build-capture/frontend/flashvm_example_frame_capture --offscreen --embedded --tick 3 "$PWD/not-read.swf" linked.ppm
ctest --test-dir build-capture -R 'offscreen|frame_capture|frontend_frame_copy' --output-on-failure
```

`--offscreen` selects timer-only software output and leaves the application's SDL
events/cursor/video alone. The selected tick still follows real-time scheduling.
The same copied PPM remains valid after SDK cleanup. SDL is still linked. Audio is
muted by this tutorial. Native video is the default without this flag. No input
injection or native OS/widget certification is implied.

`--offscreen` выбирает программный растр без окна и не читает SDL-события хоста,
не меняет курсор и не запускает video. Такт по-прежнему зависит от часов реального
времени. PPM записывается из своей копии после завершения SDK; SDL остаётся
линкуемой зависимостью. В примере звук отключён. Без флага сохраняется окно.
Инъекция ввода/приёмка native OS/widget здесь не заявляются.

| C API | Purpose / Назначение и владение |
| --- | --- |
| [fvm_desktop_run_source_offscreen](../../../frontend/common/frame.h#L95) | Synchronous FILE/MEMORY source/control/observer borrows through return; no video/events/cursor. Синхронный local/linked запуск; заимствования до возврата. [EN](../../../doc/OFFSCREEN_PLAYER.md) / [RU](../../../doc/OFFSCREEN_PLAYER.ru.md). |
| [fvm_desktop_run_source_background_offscreen](../../../frontend/common/frame.h#L100) | Explicit provider/monitor version; worker cancellation/join precedes return, pixels borrowed only inside observer. Явный provider/monitor, отмена/join до возврата. [EN](../../../doc/OFFSCREEN_PLAYER.md) / [RU](../../../doc/OFFSCREEN_PLAYER.ru.md). |

## Resource control / Ресурсное управление (v0.728)

```sh
./build-capture/frontend/flashvm_example_frame_capture --offscreen --resource-control --tick 2 flashVM/examples/player/frame_capture/start.swf resource.ppm
ctest --test-dir build-capture -R '^flashvm_resource_capture_example$' --output-on-failure
```

Флаг создаёт native-очередь одного владельца, выдаёт local session 1 CONTROL
для /timeline/ctl, post-ит один STOP после копии кадра и подтверждает доставку.
Unregister/destroy идут после runner до shutdown SDK и записи output. Сетевой
endpoint управления и GUI-ФС не создаются. HTTP исходного SWF остаётся отдельным
прежним provider. Native C-запрос не требует S-record encoding; live fixtures
дополнительно проверяют binary route. Pixels/header/exit status прежние.
Повтор флага отвергается до startup. [Контракт](../../../doc/RESOURCE_DISPATCH.ru.md).

| C API | Назначение и владение |
| --- | --- |
| [`fvm_rdispatch_create`](../../../include/flashvm/rdispatch.h#L95) | Создать ограниченную native-очередь одного владельца, без startup VM/SDK. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [fvm_desktop_resource_control_init](../../../frontend/common/resource_control.h#L29) | Зарегистрировать CONTROL endpoint; заимствование adapter в Capture живёт до unregister. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_grant`](../../../include/flashvm/rdispatch.h#L108) | Выдать CONTROL лишь доверенной локальной сессии, не произвольному remote client. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_post`](../../../include/flashvm/rdispatch.h#L115) | Скопировать пустой STOP request; принятие не равно исполнению. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [fvm_desktop_resource_control_notify](../../../frontend/common/resource_control.h#L39) | Доставить команду у владельца, вернуть intent без сохранения VM pointers. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_take`](../../../include/flashvm/rdispatch.h#L126) | Скопировать/подтвердить reply доставки, освободить identity/capacity. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_cancel`](../../../include/flashvm/rdispatch.h#L121) | На ошибке отменить queued STOP до take сохранённого результата. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_unregister`](../../../include/flashvm/rdispatch.h#L105) | После runner снять регистрацию до освобождения Capture. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
| [`fvm_rdispatch_destroy`](../../../include/flashvm/rdispatch.h#L131) | Освободить native dispatcher после подтверждения всех принятых запросов. [EN](../../../doc/RESOURCE_DISPATCH.md) / [RU](../../../doc/RESOURCE_DISPATCH.ru.md). |
## Состояние через ресурсы v0.734

При `--resource-control` owner notify копирует состояние в `/timeline/state`,
локальный наблюдатель независимо забирает и декодирует события. Снимок хранит
свои байты после завершения SDK; корень/context не удерживается. Source может
быть усечён с флагом. Тихие TURN не заполняют очередь. Сначала уничтожается
state adapter, затем control registration и dispatcher. Сетевого клиента нет.

| API | Владение / исполнение |
| --- | --- |
| [fvm_desktop_resource_state_create](../../../frontend/common/resource_state.h#L41) | Создаёт native cache, заимствуя диспетчер без старта SDK. [doc/PLAYER_RESOURCE_STATE.ru.md](../../../doc/PLAYER_RESOURCE_STATE.ru.md). |
| [fvm_desktop_resource_state_observe](../../../frontend/common/resource_state.h#L50) | Owner копирует safe point до возврата следующей команды. [doc/PLAYER_RESOURCE_STATE.ru.md](../../../doc/PLAYER_RESOURCE_STATE.ru.md). |
| [fvm_desktop_resource_state_read](../../../frontend/common/resource_state.h#L55) | Декодирует собственные event bytes в отдельную скалярную копию. [doc/PLAYER_RESOURCE_STATE.ru.md](../../../doc/PLAYER_RESOURCE_STATE.ru.md). |
| [fvm_desktop_resource_state_destroy](../../../frontend/common/resource_state.h#L61) | Закрывает регистрацию/watches до окончания жизни диспетчера. [doc/PLAYER_RESOURCE_STATE.ru.md](../../../doc/PLAYER_RESOURCE_STATE.ru.md). |
| [`fvm_rdispatch_subscribe`](../../../include/flashvm/revents.h#L53) | Открывает независимую очередь копий доверенной локальной сессии. [doc/RESOURCE_EVENTS.ru.md](../../../doc/RESOURCE_EVENTS.ru.md). |
| [`fvm_rdispatch_event_take`](../../../include/flashvm/revents.h#L65) | Забирает только эту очередь; PENDING тихого TURN не ошибка. [doc/RESOURCE_EVENTS.ru.md](../../../doc/RESOURCE_EVENTS.ru.md). |
## Экспорт кадров v0.736

Опция `--resource-frames` вместо `--resource-control` включает также native
кэш `/display/frame`: три слота по 4 MiB, всего 12 MiB пикселей. Лимит задан
в [resource_limits.h](resource_limits.h), не является скрытым default SDK.
Каждый presented frame копируется без padding; локальная metadata subscription
не pin-ит пиксели. BUSY увеличивает dropped и сохраняет старый экспорт.
Ошибки geometry/capacity/publication завершают этот пример явно. Обычный режим
и resource-control не выделяют этот кэш. Cleanup: revoke локальной сессии,
destroy frame adapter, затем state/control/dispatcher. PPM по-прежнему owned
копия после SDK teardown. Это publisher/metadata пример; CHUNK/leases описаны
в руководстве и проверяются отдельными клиентскими fixtures.

```sh
./build-player/frontend/flashvm_example_frame_capture --offscreen --resource-frames --embedded flashVM/examples/player/frame_capture/start.swf frame.ppm
```

| API | Владение / исполнение |
| --- | --- |
| [fvm_desktop_resource_frame_create](../../../frontend/common/resource_frame.h#L67) | Создаёт три слота явного размера, заимствует dispatcher. [doc/PLAYER_RESOURCE_FRAMES.ru.md](../../../doc/PLAYER_RESOURCE_FRAMES.ru.md). |
| [fvm_desktop_resource_frame_capture](../../../frontend/common/resource_frame.h#L76) | Owner копирует строки и публикует metadata без удержания borrowed pixels. [doc/PLAYER_RESOURCE_FRAMES.ru.md](../../../doc/PLAYER_RESOURCE_FRAMES.ru.md). |
| [fvm_desktop_resource_frame_read](../../../frontend/common/resource_frame.h#L86) | Декодирует metadata события в отдельную native копию. [doc/PLAYER_RESOURCE_FRAMES.ru.md](../../../doc/PLAYER_RESOURCE_FRAMES.ru.md). |
| [fvm_desktop_resource_frame_revoke](../../../frontend/common/resource_frame.h#L91) | Удаляет локальные права/watches и все pins сессии перед teardown. [doc/PLAYER_RESOURCE_FRAMES.ru.md](../../../doc/PLAYER_RESOURCE_FRAMES.ru.md). |
| [fvm_desktop_resource_frame_destroy](../../../frontend/common/resource_frame.h#L97) | Снимает регистрацию до free native pixels; dispatcher принадлежит хосту. [doc/PLAYER_RESOURCE_FRAMES.ru.md](../../../doc/PLAYER_RESOURCE_FRAMES.ru.md). |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_rdispatch_cancel` | function | [include/flashvm/rdispatch.h:121](../../../include/flashvm/rdispatch.h#L121) |
| `fvm_rdispatch_create` | function | [include/flashvm/rdispatch.h:95](../../../include/flashvm/rdispatch.h#L95) |
| `fvm_rdispatch_destroy` | function | [include/flashvm/rdispatch.h:131](../../../include/flashvm/rdispatch.h#L131) |
| `fvm_rdispatch_event_take` | function | [include/flashvm/revents.h:65](../../../include/flashvm/revents.h#L65) |
| `fvm_rdispatch_grant` | function | [include/flashvm/rdispatch.h:108](../../../include/flashvm/rdispatch.h#L108) |
| `fvm_rdispatch_post` | function | [include/flashvm/rdispatch.h:115](../../../include/flashvm/rdispatch.h#L115) |
| `fvm_rdispatch_subscribe` | function | [include/flashvm/revents.h:53](../../../include/flashvm/revents.h#L53) |
| `fvm_rdispatch_take` | function | [include/flashvm/rdispatch.h:126](../../../include/flashvm/rdispatch.h#L126) |
| `fvm_rdispatch_unregister` | function | [include/flashvm/rdispatch.h:105](../../../include/flashvm/rdispatch.h#L105) |
<!-- source-links:end -->
