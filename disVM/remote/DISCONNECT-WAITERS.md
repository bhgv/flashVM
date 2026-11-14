# Offered callers after connection loss / Ожидания после обрыва

The private Proxynet and Proxyref supervisors now observe terminal state as well
as the original absolute deadline. One existing alarm child becomes a watchdog:
it sleeps for at most 100 milliseconds per check, without creating a new thread
for each check or extending the deadline. Normal completion joins that child.
This is a polling interval, not a real-time scheduler guarantee.

Private supervisors Proxynet и Proxyref теперь наблюдают terminal state вместе
с исходным absolute deadline. Один прежний alarm child становится watchdog:
каждый sleep не длиннее 100 ms, без нового потока на каждую проверку и без
продления deadline. При completion child завершается через join. Это интервал
проверки, а не гарантия real-time scheduling.

## Completion and uncertainty / Ответ и неопределённость

An offered request whose broker/actor has retired returns `Unknown` if no reply
is available. It must not wait another sixty seconds after known closure. A call
that has not offered work can return `Disconnected`. Neither case authorizes an
automatic retry, rollback or replacement connection with old images.

Terminal publication has two nonzero phases. `Completing` rejects new calls
before native lease revocation and before active reply publication. After that
buffered reply is published, `Closed` wakes the other queued supervisors. Their
watchdogs wait through Completing and still prefer an available actual reply.
Exceptional/idle termination without an active reply can go directly to Closed.
The original absolute deadline remains a separate bound in both phases.

| State | New calls | Queued watchdogs |
| --- | --- | --- |
| Open = 0 | Validate and offer | Wait for reply, closure or deadline |
| Completing = 2 | Disconnected | Wait for active completion or original deadline |
| Closed = 1 | Disconnected | Prefer reply, otherwise Unknown for offered work |

CI on AMD64 exposed why both phases matter: publishing reply before any refusal
flag allowed immediate reentry to see an apparently open broker with revoked
leases. Conversely, publishing final closure before reply could hide successful
Close/Stop behind Unknown. The two phases preserve both boundaries without a
global lock/stop or moving the original function/variable execution owner.

Две ненулевые фазы решают обе гонки. Completing сначала запрещает новые calls,
затем actor отзывает leases и публикует buffered reply. Closed публикуется после
reply и освобождает остальные queued supervisors. Watchdog ждёт через Completing,
сохраняя original absolute deadline и приоритет готового reply. Idle/exception
без активного reply могут сразу перейти в Closed. При успешном Close/Stop
результат сохраняется, а immediate reentry уже получает Disconnected.

This does not add transaction cancellation or change raw Owner Handle ownership.
A closed endpoint may still be held by application code; its buffered requests
are ordinary managed roots until their containing objects become unreachable.
Native connection leases are revoked on closure. GC finalizers still perform no IO.

Это не transaction cancellation и не изменение ownership raw Owner Handle.
Приложение может удерживать closed endpoint; buffered requests остаются обычными
managed roots, пока содержащие их объекты доступны. Native connection leases
отзываются при closure. GC finalizers по-прежнему не выполняют IO.

## Actual acceptance / Проверка настоящих VM

`test_proxy_retirement.py` includes both network and Ref callers, in C/JIT with
fast/locked GC metadata polling. Two actual VMs communicate through a fault relay:

1. A applies the first Add exactly once, changing its original Cell from 10 to 11.
2. Relay validates that completed reply and withholds it. A forwarding observer
   acknowledges seven more requests only after sends to the real broker/actor
   mailbox complete; caller-start markers alone are insufficient.
3. Relay disconnects both directions. All eight offered callers must report
   Unknown within a three-second allowance after actual closed-state publication.
   Request deadlines remain sixty seconds; the whole client VM must exit within
   forty seconds, including its native Progs. A's final Cell must still be 11.
4. Retired-caller variants kill one of the seven queued callers before disconnect.
   Seven public completions and the abandoned supervisor/watchdog must all retire.

Проверка охватывает обычные network и Ref callers, C/JIT и оба GC polling пути.
Observer подтверждает именно actual mailbox send каждого запроса. Это семь queued
requests, а не проверка полной ёмкости mailbox. Orphan variants завершают один
queued caller до разрыва. Проверяются семь replies, завершение abandoned supervisor
и исходное A=11; request budget остаётся 60 s, whole-VM bound — 40 s.

Two independent private omissions remove only terminal-state observation from
one watchdog. The fixture detects missing completions, drains abandoned requests
with synthetic Unknown **only in omission cleanup**, joins all children and then
raises the exact missing-completion diagnostic from a separate frame. The driver
requires exit 1, that diagnostic, A=11, matching native thread lifetimes, drained
RC/GC/token ownership and strict concurrent-GC counters. A timeout, compiler error,
crash or sanitizer report cannot satisfy this control. Production never synthesizes
those cleanup replies and never bypasses the broker or actual variable actor.

Две отдельные omissions выключают только terminal-state observation одного
watchdog. Synthetic Unknown используется лишь для cleanup намеренно сломанного
private bytecode; production такой обход не выполняет. Конкретная semantic failure
обязательна вместе с clean native/GC ownership. Timeout/crash не засчитываются.

## Verified series / Проверенная серия

2026-11-14: complete ordinary CTest passes 90/90. After the reply-publication fix,
ordinary routed-results/retirement/control groups pass again 3/3. Final affected
TSan groups pass 8/8: forwarding, routed results, original queue deadlines,
guest proxies, new retirement positives and all associated selected omissions.
The standalone TSan driver also passes C/JIT × fast/locked polling, both queued
orphan variants and all three policy/watchdog omission controls.

Полный ordinary CTest: 90/90; повтор после reply-publication fix: 3/3.
Final affected TSan: 8/8, включая forwarding/routed results и обе новые watchdog
omissions. Это проверка перечисленных групп, не утверждение нового полного
91-test TSan результата. Architecture workload list остаётся 54: новая проверка
расширяет existing retirement driver. Новая CI отслеживается отдельно.


## Forced interleaving / Принудительная проверка публикации

`terminal_publication_cases.py` widens the reply-to-Closed gap by 500 ms only in
private matched bytecode. Scalar and result calls receive a corrupted peer reply;
both must return Unknown and immediately refuse the next call as Disconnected,
with no additional wire bytes. `ref_terminal_cases.py` exercises actual two-VM
Close followed by immediate Read across the same widened actor gap: Close succeeds,
Read is Disconnected, source owners drain and A remains 10.

Independent omissions remove only early Completing publication, retaining the
widened gap. Both scalar/result and actual Ref cases must fail with their exact
reentry diagnostic after exercise roots and source actors have retired. No timeout,
crash or sanitizer report is accepted. All C/JIT/fast/locked combinations are used.

В private bytecode расширяется лишь gap между active reply и Closed; это не
production задержка. Отдельные omissions удаляют только Completing. Проверяются
scalar/result reply corruption и actual Ref Close/Read, немедленный отказ,
отсутствие нового wire effect и завершение всех original source owners.
