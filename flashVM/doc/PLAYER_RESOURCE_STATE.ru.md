# Копии состояния плеера через ресурсы

Аддитивный C89 API `frontend/common/resource_state.h` наблюдает существующие
owner safe points и предоставляет READ snapshot и независимые SUBSCRIBE очереди
по пути хоста, например `/timeline/state`. [EN](PLAYER_RESOURCE_STATE.md),
[управление](PLAYBACK_CONTROL.ru.md), [файлы](RESOURCE_FILES.ru.md),
[9P2000](RESOURCE_9P.ru.md), [пример](../examples/player/frame_capture/README.ru.md).

## Снимок и события

[FVM_DesktopResourceSnapshot](../frontend/common/resource_state.h#L21) владеет копией префикса source и скалярными полями.
Указателей VM/context/root/value нет; копия пригодна после возврата runner и
завершения SDK. Epoch начинается с единицы на первом READY, увеличивается на
каждом READY нового корня, включая навигацию, и не оборачивается. Он действует
только в жизни адаптера. TURN/TICK до READY даёт BUSY. Source копируется до
[FVM_DESKTOP_STATE_SOURCE](../frontend/common/resource_state.h#L11) байт с NUL и [FVM_DESKTOP_STATE_SOURCE_TRUNCATED](../frontend/common/resource_state.h#L12). Усечение побайтовое,
не гарантирует целый UTF-8 code point. Счётчики больше Uint32 насыщаются с
[FVM_DESKTOP_STATE_COUNTERS_TRUNCATED](../frontend/common/resource_state.h#L13); ровно Uint32 max — точное значение. Адаптер не восстанавливает
переполнение, которое уже произошло в самом native host counter.

READY и TICK всегда публикуются. TURN публикует POLICY только при изменении
paused/step_pending с последнего наблюдения. Тихие TURN обновляют READ cache,
не занимая event sequence/очередь. Cache фиксируется лишь после успешной
публикации, если она нужна. Исчерпание epoch/общего publication sequence даёт
LIMIT с прежним cache; изменения таймлайна и готовые пиксели не откатываются.
Переполнение отдельной очереди отражается в её missed; после потерь запросите
новый snapshot/переоткройте поток по политике хоста. Это не постоянный replay log
и не подтверждение надёжной доставки.

TICK означает завершённый advance/presentation boundary существующего runner.
OK ответа команды означает более раннюю owner delivery, а не предсказание
последующего состояния. STEP может ждать подходящего foreground turn; PAUSE
не замораживает таймеры, input, загрузки и все свойства VM. Снимок содержит host
policy/counters, не атомарный снимок всей сцены, script playhead или пикселей.

Payload: десять канонических неотрицательных S-целых:
version, epoch, phase, paused, step_pending, movie_ticks, total_ticks,
presentations, flags, source_size; затем ровно source_size байт. Он помещается
в существующий checksummed resource reply/event envelope, не native struct.
Виды событий READY/TICK/POLICY, phase — существующие playback notification phases.
Elapsed double не передаётся; ABI вещественных чисел и домен часов не навязываются.

## API и владение

Мутации адаптера и resource handlers выполняются на owner плеера. Worker может
post/take копии через диспетчер с работающими mutex hooks и декодировать payload,
но не вызывает observe и не обращается к живому player state. Create/observe/
destroy последовательны, без reentry. Входы/выходы не перекрывают друг друга и
opaque storage. Handler читает только owner cache, VM не опрашивает.

| Функция | Контракт |
| --- | --- |
| [fvm_desktop_resource_state_create](../frontend/common/resource_state.h#L41) | Диспетчер, canonical path, выход adapter и target. Одно fixed native allocation, регистрация READ snapshot, заимствование диспетчера до destroy. Выходы только на OK. Неверный путь/дубликат/лимит регистрации возвращаются явно; NOMEM при allocation. Нет startup SDK, grant и установки notify. Выходы должны иметь отдельную стабильную память. |
| [fvm_desktop_resource_state_observe](../frontend/common/resource_state.h#L50) | Адаптер и заимствованный настоящий playback notify. Копирует скаляры/source без удержания context/root. Exact struct_size, ненулевой source, известный phase, paused/step 0/1. READY увеличивает epoch; TURN/TICK до READY BUSY. Публикует READY/TICK либо изменённый TURN policy, затем cache. Ошибка сохраняет cache; статус publish передаётся. Без allocation/pump/I/O/консоли и изменения policy. |
| [fvm_desktop_resource_state_read](../frontend/common/resource_state.h#L55) | Counted payload и native snapshot output. Полная проверка versioned canonical integers и точной длины source. Неизвестные schema/flags, неверные целые/booleans/phase, внутренний NUL, неверный truncation extent и хвост дают INVALID без изменения выхода. Без VM, заимствованного результата и allocation; подходит клиенту с собственной копией байт. |
| [fvm_desktop_resource_state_destroy](../frontend/common/resource_state.h#L61) | Указатель на адаптер после остановки notify/consumers. Owner снимает точное поколение регистрации, отбрасывает watches, освобождает только адаптер и обнуляет указатель. BUSY внутри resource callback; ошибки owner/регистрации сохраняют указатель. Старые queued jobs затем дают UNKNOWN; хост должен их take/disconnect. Диспетчер/сессия/SDK остаются хосту. |

[FVM_DESKTOP_STATE_SNAPSHOT](../frontend/common/resource_state.h#L7) требует READ и пустой request. До первого успешного observe — terminal
BUSY; непустой payload INVALID. Хост явно выдаёт READ и/или SUBSCRIBE и решает,
можно ли сообщать source этой аудитории. SUBSCRIBE без READ допускает события.
Сессия не берётся из wire claim, OS filesystem не открывается.

Вызывайте observe перед fvm_desktop_resource_control_notify в том же host notify:
состояние копируется до применения возвращаемой команды. Нельзя pump control
снаружи control notify — прежний BUSY контракт остаётся. Observe не pump-ит,
сохраняя FIFO с одним job на границу. При STOP после ошибки наблюдения обычная
очистка плеера всё равно join-ит загрузки и освобождает managed roots.

Сначала закрыть file/9P clients, остановить notify, destroy state adapter,
unregister control, take/disconnect незавершённые запросы, destroy диспетчер.
Адаптер не продлевает runner/SDK. [GC:] Корень во время notify удерживается
существующим pin runner; новых managed references, roots и GC работы нет.

## Пример и сборка

Режим `--resource-control` примера frame_capture теперь публикует и локально
декодирует независимые state events перед постановкой STOP. Проверяет decode/
missed, сохраняет собственный snapshot после SDK teardown и удаляет адаптер до
диспетчера. Обычный режим захвата новых ресурсов не создаёт.

```sh
cmake -S flashVM -B build-state -DFVM_BUILD_DESKTOP_FRONTEND=ON -DFVM_BUILD_TESTS=ON
cmake --build build-state --target flashvm_frontend_resource_state flashvm_example_frame_capture flashvm_frontend_host_abi
ctest --test-dir build-state -R '^(flashvm_resource_state_contract|flashvm_resource_capture_example|flashvm_frontend_host_abi)$' --output-on-failure
```

Нужны documented static SDL SDK/provider paths и build definitions из
[руководства host](DESKTOP_FRONTEND.ru.md) и
[платформенной приёмки](PLATFORM_VALIDATION.ru.md). Standalone contract не
запускает SDL/устройства: независимые очереди, тихие TURN, строгие prefix decode,
epoch, усечение source/counters, отказ allocation, неверный notify, исчерпание
epoch и queued stale teardown. Ручные fixture notifications проверяют контракт,
а фактический playback — рабочий пример и live-player tests. OS mount и запуск
на чужой платформе не подразумеваются. Все девять стадий открыты.

## Плеер через 9P v0.735

Дополнительный live fixture передаёт каждую pause/resume/step/stop команду через
настоящие version/attach/walk/open/write/read/clunk кадры 9P2000 в файлы операций
control endpoint. Отдельные FS/9P соединения с доверенными сессиями 7 и 8 имеют
свои subscriptions на state endpoint. Права CONTROL выдаются только первой
сессии; обе получают READ/SUBSCRIBE состояния. Клиентский uname прав не выдаёт.

На каждом notify owner сначала observe-ит состояние, затем тестовые клиенты
проверяют события через самостоятельные binary decoders, после чего owner
доставляет не больше одной queued команды. Отсутствующее событие оставляет
pending read, не ложный EOF. Оба клиента должны получить одинаковое число
READY/TICK/policy events без потерь; payload сравнивается с живой owner-копией.
При навигации ожидаются два root epoch, в остальных начавшихся сценариях один.
Для успешных sequence/navigation число TICK events равно completed host ticks.

Используется существующий independent SWF generator и покомпонентная проверка
реальных RGBA пикселей: исходный кадр, одиночный STEP, paused redraw, следующий
STEP, FILE и внутренний MEMORY buffer, HTTP root navigation и STOP во время
активной загрузки loadVariables с cancel/join до очистки. Затем уже после
возврата runner клиент читает cache через state READ operation и owner pump:
handler использует только native snapshot, без умершего context/root.

```sh
cmake --build build-state --target flashvm_frontend_resource_9p_player
ctest --test-dir build-state -R '^flashvm_resource_9p_player_' --output-on-failure
```

Тестовый C89 клиент: [resource_9p_client.c](../frontend/tests/resource_9p_client.c),
fixture: [frame_capture.c](../frontend/tests/frame_capture.c). Два теста используют
interpreter и actual native JIT там, где он доступен; отсутствие backend явно
SKIP, не успех. HTTP зависит от собранного provider, локальные варианты остаются
без него. In-memory transport проверяет message/file/owner интеграцию, не
production TCP/UDP listener, auth protocol, ОС mount или физический дисплей.
Предыдущий отдельный 9P wire fixture покрывает настоящий fragmented TCP loopback.

## Проверка серии v0.734–v0.735

На Linux x86-64 прошли 45 выбранных native проверок и 10 shared-host проверок.
Native конфигурация включала socket HTTP; shared — без HTTP providers, с явной
проверкой отсутствующей возможности. Прогоны использовали offscreen/dummy,
не физические экран и аудиоустройство. State contract и новые state/protocol/
9P-client модули в реальном player fixture прошли ASAN/UBSAN для interpreter и
JIT. Основные VM/SDL архивы в этом fixture обычные, leak enumeration отключён;
полная память VM этим прогоном не сертифицирована.

C89 state/dispatcher contract кросс-слинкован для ARM64 Linux musl, RISC-V64 Linux
musl и Windows x64 GNU; C++98 player host ABI probe скомпилирован в target objects.
Эти target binaries не запускались. Пример самостоятельной проверки модуля:

```sh
zig cc -target aarch64-linux-musl -std=c89 -pedantic-errors \
  -IflashVM/include flashVM/frontend/tests/resource_state.c \
  flashVM/src/protocol/rdispatch.c flashVM/src/protocol/srecord.c -o state-arm64
```

Fixture включает настоящий resource_state.c с управляемым fault injection; SDL
ему не нужен. Это не кросс-сборка всего плеера. Для player host нужен toolchain
и static SDL SDK той же целевой платформы/CPU; выполните конфигурацию из
[DESKTOP_FRONTEND.ru.md](DESKTOP_FRONTEND.ru.md) с явными CMAKE_TOOLCHAIN_FILE,
SDL2_DIR и профилем player-jit.defs, затем соберите flashvm_player_host и
flashvm_example_frame_capture (desktop). Android требует описанной отдельно
Activity/surface интеграции. Реальные target запуск и приёмка остаются отдельными.
