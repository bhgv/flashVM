# Отдельные diagnostic mailbox — первый исполняемый срез

Copyright (c) 2026 bhgv

Read-only scalar probes обслуживает permanent native owner resident. Это
работающий внутренний механизм своей VM; external transport, source/stack capture,
capabilities для управления, pause/resume/step и priority пока не реализованы.
Controller, collectors и native I/O helpers не входят в этот resident registry.

## Guest API

Matched `Processctl` предоставляет три новых вызова:

| Вызов | Результат |
| --- | --- |
| residentrequest(exact ID, cookie) | status, ticket; accepted request копирует signed-big cookie |
| residentreply(exact ID, ticket) | status, owner PID, cookie, dispatches, turns, waits |
| residentforget(exact ID, ticket) | status; освобождает только diagnostic credit |

Ticket привязан к original requester PID и lifetime конкретного resident mailbox.
Он монотонен, не переиспользуется после consume/forget, исчерпывается на signed-big
max. Это не control capability и не разрешение выполнять guest callback.
Другой Prog не может прочитать либо забыть чужой ticket, даже получив его число.
Старые методы Processctl сохраняют свои сигнатуры; caller компилируется по matched
interface. Все effects выполняются до guarded result publication; её retry не
повторяет submit, consume либо forget. Reply имеет только scalars, без heap pointers.

| Status | Значение |
| --- | --- |
| DiagReady = 1 | Submit принят либо Ready reply поглощён; forget освободил credit |
| DiagPending = 0 | Owner ещё не ответил; polling не поглощает credit |
| DiagInvalid = -1 | Некорректный scalar argument |
| DiagFull = -2 | Все восемь credits заняты pending/ready/closed replies |
| DiagClosed = -3 | Owner retired; новые requests запрещены; pending reply стал terminal |
| DiagMissing = -4 | Exact target/ticket больше не зарегистрирован |
| DiagDenied = -5 | Ticket принадлежит другому requester |
| DiagExhausted = -6 | Lifetime ticket sequence исчерпана |

Submit refusal возвращает ticket=0. Любой non-Ready reply обнуляет весь payload.
Ready и Closed reply освобождают credit ровно один раз; повторное чтение — Missing.
Closed не уничтожает уже готовый snapshot: Ready можно прочитать до reaping target.
Если target снят с registry раньше polling, возвращается Missing; запрос не удерживает
Prog, native thread либо его heap ради debugger. Здесь нет межпроцессного transport
или generation authentication для другой VM.

## Изоляция, квота и owner

Каждый resident получает отдельный native mailbox с mutex и восемью заранее
выделенными slots. Pending, Ready и Closed используют одну квоту. Submit/reply/
forget не выделяют память. Малый atomic pending hint оставляет обычный путь без
mailbox lock, если diagnostic requests нет. Обычные channels, cancellation/reply
FIFO и diagnostic slots не используют одни и те же сообщения или payload buffers.

Submit под registry guard удерживает slot, копирует request и сигналит только
private readiness condition адресата. Owner просыпается даже при блокировке на
обычном канале; очередь выбирает старейший pending ticket, включая reuse slots.
В одном servicing point выполняются максимум два ответа, затем ordinary dispatch
получает возможность продолжиться. Pending queries обслуживаются перед turn и
после native readiness wake. Бюджет не означает hard realtime latency guarantee.

Owner sampler проверяет permanent TLS/context identity и копирует только native
PID/progress. Он не читает чужой Prog/REG, не вызывает getter, remote RPC либо guest
code и не обходит heap. Counter meanings остаются прежними: callbacks/completed
turns/readiness waits, не инструкции, syscalls либо CPU time. Captured native
sample — наблюдение в owner boundary, не atomic snapshot всего процесса.

Во время блокирующего native callback owner может не достигнуть servicing point.
Тогда reply остаётся Pending; foreign thread не исполняет запрос вместо него.
Забывание diagnostic query не отменяет native sleep/channel/RPC. Pause для
исследования stack/variables потребует отдельной safe-boundary/root publication.

## Lifetime и блокировки

Registry guard удерживается перед mailbox lock; mailbox code не берёт registry
или heap guards и не вызывает callbacks. Producer отпускает mailbox mutex до
получения wake mutex. Idle predicate читает atomic pending hint. Ни один новый
lock не является global VM admission/exclusive или stop-the-world.

Завершение requester освобождает его pending/ready/closed credits во всех ещё
зарегистрированных boxes. При отсутствии любых diagnostic credits atomic hint
пропускает registry cleanup целиком; original requester уже завершил submit calls. Scan — O(number of residents × 8) под metadata guard;
индекс requester credits и дополнительные per-client/global quotas остаются
следующим улучшением при масштабировании. Это не общий constant-time cleanup.

Owner retirement закрывает pending tickets; готовые snapshots сохраняются до
consume/forget/reaping. Полный service stop и startup refusal уничтожают native
boxes. В report `NATIVEDEBUGMAILBOX` после joins должны быть live=0, requests=0,
pending=0, created=destroyed. Эти native scalars не являются GC roots. Ticket
от старого target не переадресуется следующему lifetime ID.

## Acceptance

`test_debug_mailbox.py` запускает реальные VM в C/JIT × fast/locked polling.
Шесть гостей проверяют blocked-channel replies, exact owner/cookie (включая
signed-big min/max), восемь credits, foreign requester denial, ticket consume/
forget/reuse, автоматическую очистку после sender retirement, параллельные 32
ordinary transfers и diagnostic queries, pending native sleep и target retirement.
Отдельный watcher читает shared 48-byte reply ADT с соседними canaries во время
publication. Native wrapper подтверждает actual bound owner и бюджет два.

Native queue unit использует ту же production implementation: все Ready replies
продолжают занимать quota; после slot reuse FIFO выбирает старые pending tickets;
invalid budget/arguments не портят очередь; denied/non-ready outputs нулевые;
close idempotent, сохраняет Ready и закрывает Pending. Его дополнительный native
box учитывается отдельно и полностью уничтожается.

Два private omission controls скрывают diagnostic pending wake predicate либо
пропускают requester cleanup. Они должны дать точный DETECTED diagnostic после
normal application exit и полного native/GC retirement. Compile/crash/timeout/
sanitizer/nonzero exit не являются успешным обнаружением. Workload debug-mailbox
включён в обязательную architecture matrix; число workloads выросло до 57.

Финальные восемь затронутых групп проходят 8/8 обычно и 8/8 под TSan: process
control, live registry, debug mailbox, receiver/native cancellation, retirement
storage, table transfer и table retirement. После добавления empty-credit fast
path повторно проходят process control/debug mailbox — 2/2 обычно и 2/2 TSan.
Actual mailbox lifecycle report нормализуется в JSON; три normalizer unit checks
проходят. Это выбранная локальная приёмка, не полный новый architecture/runtime
прогон. Предыдущая база90299f44 прошла все шесть CI jobs, включая AMD64/ARM64/RV64;
новые изменения требуют своего CI.
