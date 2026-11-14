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
