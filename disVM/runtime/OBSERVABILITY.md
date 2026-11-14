# Runtime observability / Наблюдаемость всей VM

Copyright (c) 2026 bhgv

Область следующего этапа: каждый режим, загруженный модуль, Prog/native thread,
канал, значение/переменная, remote proxy, collector и native/managed allocation
должны иметь статистику и проверяемый путь диагностики. Это последовательная
работа над live runtime; данный документ не объявляет live debugger готовым.

## Основа и пробелы

| Область | Имеющаяся основа | Следующее расширение |
| --- | --- | --- |
| Режимы | native policy, backend и CLI diagnostics | конфигурация, build/profile identity, включённые collectors/JIT |
| Модули | counters, SBL/Debug metadata Inferno | live module instances, generation, symbols/PC mapping |
| Потоки | NATIVEWORKERS/GUEST, Processctl, owner monitor | единые Prog/thread cards, state/wait reason, stack/location |
| Каналы | queue diagnostics и broker deviations | automatic entity registration, buffered/rendezvous state, waiters |
| Переменные | entry scalar inspection, Debug types | owner-bound typed read, aliases, revision, bounded graph projection |
| Remote proxies | PROXYHOST/PROXYGC, broker metadata | connection/epoch/handle/lease/request cards, deadlines and causal history |
| GC | native policy, RC/collector/queue counters | per-collector progress, retained roots, pending release causes |
| Память | heap/type/module/file/root probes | allocations/bytes/peaks by owner/type/module, native buffers and descriptors |

`upstream/module/debug.m` и `upstream/appl/lib/debug.b` дают символы/типы, но
сами по себе не обеспечивают live /prog, debugger transport и безопасное чтение
нашего concurrent heap. `broker/` уже умеет copied metadata и bounded deviation
queues; automatic instrumentation всех VM channels там пока отсутствует.
`heap_snapshot.h` требует защищённый lifetime и frozen/excluded metadata для
нескольких полей: его нельзя выдавать за concurrent live graph snapshot.

## Общий контракт live-наблюдения

- Stable entity ID + generation + VM/build identity; адрес не служит authority.
- Counter, событие и snapshot различимы. Для каждого значения известны источник,
  момент/sequence, область согласованности и причина unavailable/truncated.
- Состояние публикует владелец. Inspector не исполняет getter, callback или remote
  RPC внутри объекта лишь ради чтения его состояния.
- Короткие metadata guards и owner mailboxes; никакого global stop/exclusive.
  Pause/step одного Prog не должен лишать диагностический сервис ответа.
- Чтение переменных/stack требует lifetime guard и owner-bound snapshot. GC roots,
  native pins, deferred releases и shared aliases показываются раздельно.
- Bounded event queues, dropped-event counts, subscriptions/filtering и quotas.
  Медленный observer не блокирует приложение и не удерживает его heap навечно.
- Debugger mutations, pause/step/breakpoint, memory edits и обычное read-only
  наблюдение имеют отдельные permissions и явный результат операции.

## Первый срез: existing reports → JSON

`../tools/runtime_stats.py [report]` читает UTF-8 report или stdin и выдаёт
`disvm-runtime-observation-v1`. Сохраняются все NATIVE/PROXY/MANAGED families,
повторные entity records, event labels и итоговые instructions/GC counters.
Значения остаются strings, чтобы uint64 identity/counters не теряли точность
в JSON consumers. Application output не становится диагностическим record.

Некорректный record исключается целиком, публикуется ошибка с номером строки,
`complete=false`, exit status 1. `complete` означает корректность разбора, а не
полноту наблюдения всей VM. `coherent_live_snapshot=false` задаётся явно.
Это нормализатор существующих reports, не live transport и не heap inspector.

Проверки: `python3 -m unittest discover -s disVM/tools -p test_runtime_stats.py`.

## Последовательность реализации и приёмки

1. Общие entity cards/schema и adapters существующих counters; CLI/JSON consumer.
2. Live service: runtime/modes/modules/Prog/thread/channel/GC/memory metadata;
   epoch/lifetime consistency, nonblocking observer, events/backpressure tests.
3. Typed variables, stacks и proxy request/lease/root graph с ограничениями depth,
   bytes и временем; race с GC, завершением Prog, unload и disconnect.
4. Breakpoints, pause/resume/step и source mapping на owner-bound threads;
   debugger disconnect/exception и независимое продолжение остальных Prog.
5. Подключение IDE к тому же API, export/import diagnostic captures и сравнение
   запусков. Acceptance C/JIT × GC polling, AMD64/ARM64/RV64 по доступной матрице.

Каждый adapter принимается на реальном работающем объекте и при его разрушении,
а не только на synthetic counters. Ошибки наблюдателя не должны менять результат
программы; race/GC/retirement omissions обязаны обнаруживаться семантически.

## Live resident registry: первый native adapter

`dvm_resident_observe` в `managed_workers.h` копирует metadata работающего native
resident registry. В странице 1..256 карточек: stable lifetime ID сервиса, permanent
Prog PID и опубликованный признак done после TLS destruction. IDs отсортированы;
`after` задаёт cursor, `more` сообщает усечение. Reaped записи могут исчезать между
страницами; несколько страниц не объявляются единым atomic snapshot.

Сервис удерживается вызывающим до возврата; concurrent service stop запрещён
контрактом lifetime. Выход — disjoint caller-owned native storage. Invalid arguments
либо unavailable service дают отказ без изменения outputs. Короткий service.sync
защищает только копирование metadata; Prog/REG, owner-only counters и managed heap
не читаются, target code не исполняется и heap admission не запрашивается.
Это C API адаптера; guest/IDE transport, wait reasons и stack views ещё впереди.
Collector/file-helper threads требуют собственных registry adapters.

`test_live_registry.py` перехватывает только private Sys callback и читает реальные
карточки при создании/завершении восьми guest threads. Проверяются ascending IDs,
PID, bounded page, end cursor, refusal without output mutation и strict native/GC
retirement evidence. C/JIT × fast/locked polling проходят обычно и под TSan.
Workload `live-registry` включён в architecture matrix; список вырос до 56.

Приёмка live adapter: пять затронутых групп process control/cancellation/retirement/
observation прошли 5/5 обычно и 5/5 под TSan. После расширения проверок cursor,
NULL outputs и invalid capacity вновь прошли четыре C/JIT/poll сочетания TSan.
Это не полный прогон всей runtime suite и не готовый debugger transport.

## Guest/CLI доступ к resident cards

`Processctl.resident(after: big)` возвращает `(status, id, pid, done)` через guarded
pure-scalar result publication. Status: 1 — карточка, 0 — текущий конец страницы,
-1 — invalid/unavailable/unrepresentable identity. При end/refusal все поля данных
нулевые. Cursor неотрицателен; lifetime ID возвращается только в signed-big range.
IDs — read-only metadata, не authority для kill, debug либо чтения памяти.

Это перечисление resident helpers своей VM. Controller не входит в этот реестр;
caller PID доступен отдельно через pid(). Collector и native I/O helper registries
ещё не подключены. Нет attach к другой OS process/VM и нет coherent multi-page
snapshot. Guest code должен быть скомпилирован с matched Processctl interface.

CLI/module пример: `disVM/examples/runtime-observe/main.b`. Он печатает свой
NATIVEOBSERVER PID и доступные NATIVERESIDENT cards. При отдельном запуске без
spawned guests реестр может быть пуст. Для наблюдения приложения этот модуль
загружается в наблюдаемую VM; внешний диагностический transport — следующий слой.
Оба record family уже понимает JSON normalizer `disVM/tools/runtime_stats.py`.

Acceptance удерживает восемь реальных гостей до окончания перечисления, сверяет
точные PID, unique ascending IDs и done=0. Tuple направляется в общую scalar ADT
с соседними canaries, пока другие гости читают поля. Проверяются zeroed end/refusal,
полный join и native/GC evidence в C/JIT × fast/locked polling. Связанные пять групп
прошли 5/5 обычно и 5/5 TSan; расширенный shared-reader сценарий прошёл повторно
во всех четырёх TSan сочетаниях. Workload остаётся live-registry в общей CI.

## Owner-published resident phases

`Processctl.residentstate(after)` расширяет результат resident до
`(status, id, pid, done, phase)`; старый resident сохраняет прежний tuple.
Cursor/status/zeroed refusal остаются прежними. CLI печатает phase_id и имя.

| Phase | Значение | Интервал владельца |
| --- | --- | --- |
| Starting | 0 | Старт native thread и attach до первого dispatch |
| Dispatch | 1 | Внутри turn callback, включая его native calls |
| Scheduler | 2 | Между callback: admission, reaping и scheduler work |
| WaitWork | 3 | Dedicated readiness wait; возможны spurious wakeups |
| Teardown | 4 | Dispatch окончен; очистка owner/context/TLS |
| Done | 5 | TLS cleanup завершён; native slot ещё может ожидать join |

Владелец публикует phase relaxed atomic store без allocation, дополнительных
metadata locks либо callbacks. Registry guard удерживает slot во время atomic
copy; он не останавливает работу наблюдаемого гостя. Phase не публикует managed
memory, не даёт lifetime pin или права читать heap. Интервал может измениться
сразу после чтения; короткие фазы можно пропустить. Done публикуется вместе с
completion под registry guard. Несколько карточек/страниц не coherent snapshot.

WaitWork означает ожидание готовности resident, а не конкретный channel/I/O
или OS CPU state. Native call внутри dispatch может сам ждать; поэтому Dispatch
нельзя трактовать как «сейчас исполняет инструкции». Конкретные wait causes,
queued requests и source/stack location требуют собственных adapters.

Acceptance удерживает восемь реальных гостей на rendezvous channel, ожидает
WaitWork у всех и проверяет self-observation Dispatch до/после wake. После wake
каждый guest остаётся жив до killwait, затем выполняется полный native join.
Два private controls меняют только copied WaitWork либо Dispatch на Scheduler;
оба должны дать точный DETECTED diagnostic после чистого завершения всех гостей.
Compile/crash/timeout/sanitizer failure не принимаются за semantic detection.
Общий результат с phase пишется также в shared scalar ADT между canaries, пока
другие гости читают поля. Старый resident сверяется на том же held registry.

Пять затронутых групп process control/cancellation/retirement/observation прошли
5/5 обычно и 5/5 под TSan. C/JIT × fast/locked polling и два semantic controls
входят в существующий workload live-registry; workload count остаётся 56.
CLI module с именами phases компилируется и запускается; его реальный report
нормализуется в JSON, три unit checks normalizer проходят. Полный новый прогон
всей runtime suite и вся архитектурная матрица этим не объявляются.

## Copied resident activity / Причина ожидания

`Processctl.residentactivity(id: big)` читает exact положительный lifetime ID:
`(status, activity)`. Status 1 — карточка найдена, 0 — отсутствует в registry,
-1 — invalid/unavailable. Без найденной карточки activity=0. Это не cursor API:
если ID исчез, следующая сортированная карточка не подменяет запрошенную.
Старые resident/residentstate tuples сохраняются; CLI показывает activity отдельно.

| Activity | Code | Последнее опубликованное состояние |
| --- | --- | --- |
| Unknown | 0 | Неизвестное значение |
| Ready | 1 | Prog ready; не означает выполнение на CPU |
| Send / Receive / Alt | 2 / 3 / 4 | Регистрация ожидания канала |
| Native | 5 | Native continuation отпустил admission |
| Table | 6 | Кооперативное ожидание table admission |
| Child | 7 | Ответ на ожидание retirement/cancellation ребёнка |
| Debug / Exiting / Broken | 8 / 9 / 10 | Соответствующее retained Prog state |

До startup registrar привязывает к Prog native scalar cell в его resident slot.
Разрешённые переходы dvm_runqueue_set_state зеркалируют туда только scalar через
relaxed atomic store. Slot живёт до native join и дольше Prog; pointer на cell
не GC root и не передаётся наблюдателю. Inspector копирует native cell под guard
реестра, не читая Prog. Отсутствующая cell сохраняет прежний путь публикации.
Код child-wait теперь объявлен в общем runqueue header без изменения значения.

Phase и activity читаются независимо и не образуют общей atomic транзакции.
Они могут меняться сразу после копирования; Done-карточка хранит последнее activity,
а не обещание живого ожидания. Missing означает отсутствие карточки, не доказательство
завершённого OS join. Sleep/RPC/I/O, конкретный channel/table, target PID, duration,
source location и stack этим API не раскрываются. Debug code не добавляет pause.

Actual acceptance удерживает send/receive/alt/table waiters и native sleeper,
проверяет точные activity и Ready после реального завершения операций. Payloads
17/42/73 и table write99 должны сохраниться. Native sleeper создаётся первым,
убирается при оставшихся поздних IDs; explicit peer dispatch даёт rootless reaping
progress, затем exact-ID query обязан вернуть missing/zero. Сам observer не
запускает target code и не форсирует reaping. Все пять native slots затем joined.

Проверяются invalid IDs, uint63 max absent ID, общие activity result fields с
canaries и параллельными readers. Scalar arrays в fixtures инициализируются явно:
allocation без initializer не обещает нулевые scalar bytes. Private ALT activity
substitution обязан дать точный DETECTED diagnostic после полного retirement.
Отдельный native queue unit удаляет реальный mirror store и требует точную ошибку;
все двенадцать ownership/observation controls проходят. Новая real-VM матрица
проверяет шесть activity (Ready и пять waits); Child/Debug/Exiting/Broken не получают
здесь отдельного нового live acceptance, только явную классификацию публикаций.

Финальные семь затронутых групп проходят 7/7 обычно и 7/7 под TSan: process control,
receiver/native cancellation, live registry, retirement storage, table transfer
и table retirement. Live registry включает C/JIT × fast/locked polling, shared
activity outputs, exact-ID gap и все три copied-data omission controls. Native
queue unit с двенадцатью controls проходит отдельно; CLI report нормализуется,
три unit checks JSON normalizer проходят. Workload count остаётся 56. Полная новая
runtime/architecture suite и live-проверка всех одиннадцати activity не заявляются.

## Progress и сводка resident registry

`residentprogress(exact id)` возвращает `(status, dispatches, turns, waits)`.
Статусы и нулевые missing/refusal outputs совпадают с residentactivity. Dispatches —
входы в resident callback (включая idle/maintenance), turns — завершённые guest
callbacks, waits — входы в dedicated readiness condition wait, включая spurious
wakeups. Это не число инструкций, syscall count, wall/CPU time либо wait duration.

Каждый counter имеет одного writer и насыщается на signed-big max; повторный
increment на максимуме не делает значение отрицательным. Inspector читает только
новые native atomic publications, а не owner-only turns/owner statistics. Completed
turns читаются acquire перед dispatches: release completed publication следует
после соответствующего dispatch, поэтому copied turns <= dispatches. Вся тройка
не является atomic snapshot; фазы/activity и другие запросы независимы.

`residentsummary()` возвращает status и coherent created/registered/reaping/reaped/
registered-done под metadata guard. Выполняется created = registered + reaping +
reaped, done <= registered. Registered включает завершённые слоты; reaping — слоты,
снятые с linked registry, но ещё принадлежащие reaper через join и отложенный учёт.
Reaped — учтённые rootless joins. При refused rootless leave слоты и reaping count
возвращаются вместе. Shutdown joins могут оставаться registered до удаления сервиса;
это сводка registry slots, не универсальный live OS thread count.

Private join interception читает actual summary между unlink и registrar join;
скрытие copied reaping count обязано нарушить partition, затем штатно завершить
все native/GC owners. Новый guest test проводит восемь владельцев через четыре
pulse/ack цикла и пять возрастающих progress readings. Девятый native guest
непрерывно читает общие progress/summary ADTs во время guarded публикаций; canaries
и отдельные scalar values сохраняются. Он не сравнивает сумму независимых reads.
32-byte progress и 48-byte summary results проверяются в shared storage. Invalid/
missing IDs обнуляют payload. Suppressed completed-turn publication даёт точный
DETECTED diagnostic после полного join девяти гостей. Boundary probe проверяет
насыщение counter на LLONG_MAX. Всё остаётся в workload live-registry.

Согласованный следующий control plane и текущая матрица готовности thread/memory/
environment/VFS/C/Limbo APIs: [CONTROL-PLANE.md](CONTROL-PLANE.md). Отдельные debug
mailbox, cooperative pause и source/stack capture пока являются следующим этапом.

Финальные семь групп проходят 7/7 обычно и 7/7 TSan: process control, live registry,
receiver/native cancellation, retirement storage, table transfer/retirement.
Live registry покрывает C/JIT × fast/locked polling, четыре copied-card omissions
и отдельное скрытие in-flight summary. Все negative cases требуют normal exit,
точный diagnostic и полный native/GC retirement. CLI actual report с нулевым
helper registry нормализуется в JSON с точной summary; три normalizer units
проходят. Architecture workload count остаётся 56; полный новый runtime/host
matrix этим не заявляется.


## Отдельные diagnostic mailbox

Первый работающий request/reply/forget путь: [DIAGNOSTIC-MAILBOX.md](DIAGNOSTIC-MAILBOX.md).
Он использует собственные fixed native credits, requester identity и owner servicing,
не ordinary message FIFO. Pending wake и requester-retirement omissions проходят
семантическую приёмку с полным native/GC drain. Read-only scalar sample не объявляет
готовыми stack/typed-variable capture, внешний debugger либо pause/resume.


## Owner-bound location и Limbo reader

Реализована копия module name/path и next original Dis PC из live TLS своего
owner в ACTIVE, до освобождения claim. Snapshot доступен через nonconsuming
ticket projections, сохраняется после module return/instance drop и не удерживает
Module pointers. Полные контракты: [EN](OWNER-LOCATION.md) / [RU](OWNER-LOCATION.ru.md).
Reusable reader — debug/residentdebug.b; пример с отдельными RU/EN инструкциями —
examples/resident-debug/. Это не полный stack, unique module catalog либо pause.

Bounded owner-produced stack prefixes and Limbo reader/example now work: [RU](OWNER-STACK.ru.md) / [EN](OWNER-STACK.md).
Typed variables, pause/resume and external debugger service remain separate work.

Live arena accounting API and Limbo view/example: [EN](MEMORY-STATISTICS.md) / [RU](MEMORY-STATISTICS.ru.md).
One O(1) native metadata snapshot; excludes object/type/origin catalog and off-arena/RSS accounting.


Retained source continuation debugging and on-demand GC-root census: [EN](DEBUGGER.md),
[RU](DEBUGGER.ru.md). Whole-function confirmed-JIT-defect quarantine: [EN](JIT-QUARANTINE.md),
[RU](JIT-QUARANTINE.ru.md). Synchronous admitted integer/basic REAL RX faults now support internal CPU capture:
[EN](JIT-SIGNALS.md), [RU](JIT-SIGNALS.ru.md). Arbitrary unsafe native faults remain
outside recovery; GDB is optional.

Base FP/SIMD snapshots and REAL fault policy: [EN](JIT-FP.md), [RU](JIT-FP.ru.md).
