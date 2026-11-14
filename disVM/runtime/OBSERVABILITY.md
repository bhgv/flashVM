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
