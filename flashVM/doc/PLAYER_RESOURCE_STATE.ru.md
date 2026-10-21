# Копии состояния плеера через ресурсы

Аддитивный C89 API `frontend/common/resource_state.h` наблюдает существующие
owner safe points и предоставляет READ snapshot и независимые SUBSCRIBE очереди
по пути хоста, например `/timeline/state`. [EN](PLAYER_RESOURCE_STATE.md),
[управление](PLAYBACK_CONTROL.ru.md), [файлы](RESOURCE_FILES.ru.md),
[9P2000](RESOURCE_9P.ru.md), [пример](../examples/player/frame_capture/README.ru.md).

## Снимок и события

FVM_DesktopResourceSnapshot владеет копией префикса source и скалярными полями.
Указателей VM/context/root/value нет; копия пригодна после возврата runner и
завершения SDK. Epoch начинается с единицы на первом READY, увеличивается на
каждом READY нового корня, включая навигацию, и не оборачивается. Он действует
только в жизни адаптера. TURN/TICK до READY даёт BUSY. Source копируется до
FVM_DESKTOP_STATE_SOURCE байт с NUL и SOURCE_TRUNCATED. Усечение побайтовое,
не гарантирует целый UTF-8 code point. Счётчики больше Uint32 насыщаются с
COUNTERS_TRUNCATED; ровно Uint32 max — точное значение. Адаптер не восстанавливает
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
| `fvm_desktop_resource_state_create` | Диспетчер, canonical path, выход adapter и target. Одно fixed native allocation, регистрация READ snapshot, заимствование диспетчера до destroy. Выходы только на OK. Неверный путь/дубликат/лимит регистрации возвращаются явно; NOMEM при allocation. Нет startup SDK, grant и установки notify. Выходы должны иметь отдельную стабильную память. |
| `fvm_desktop_resource_state_observe` | Адаптер и заимствованный настоящий playback notify. Копирует скаляры/source без удержания context/root. Exact struct_size, ненулевой source, известный phase, paused/step 0/1. READY увеличивает epoch; TURN/TICK до READY BUSY. Публикует READY/TICK либо изменённый TURN policy, затем cache. Ошибка сохраняет cache; статус publish передаётся. Без allocation/pump/I/O/консоли и изменения policy. |
| `fvm_desktop_resource_state_read` | Counted payload и native snapshot output. Полная проверка versioned canonical integers и точной длины source. Неизвестные schema/flags, неверные целые/booleans/phase, внутренний NUL, неверный truncation extent и хвост дают INVALID без изменения выхода. Без VM, заимствованного результата и allocation; подходит клиенту с собственной копией байт. |
| `fvm_desktop_resource_state_destroy` | Указатель на адаптер после остановки notify/consumers. Owner снимает точное поколение регистрации, отбрасывает watches, освобождает только адаптер и обнуляет указатель. BUSY внутри resource callback; ошибки owner/регистрации сохраняют указатель. Старые queued jobs затем дают UNKNOWN; хост должен их take/disconnect. Диспетчер/сессия/SDK остаются хосту. |

SNAPSHOT требует READ и пустой request. До первого успешного observe — terminal
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
