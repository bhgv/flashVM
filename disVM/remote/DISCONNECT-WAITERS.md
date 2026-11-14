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

Already completed replies take priority when the watchdog wakes. Broker/actor
publishes the active request's buffered reply **before** its closed flag. This
preserves successful Stop/Close results while releasing the other queued callers.
The supervisor checks the reply again when its watchdog wins the outer alt.
Exceptional termination can publish closure without a reply; that remains Unknown.
Caller-side canwait, owner-bound execution, native TLS REG and permanent Prog
thread identity are unchanged. No global stop/exclusive profile is used.

Если offered request не получил reply, а broker/actor уже завершён, caller
получает `Unknown`, не ждёт оставшиеся шестьдесят секунд. До offer допустим
`Disconnected`. Автоматический retry, rollback или перенос старых images на
другое соединение не разрешены.

Готовый reply имеет приоритет при пробуждении watchdog. Broker/actor сначала
публикует buffered reply активного запроса и лишь затем closed flag. Поэтому
успешные Stop/Close сохраняют свой результат, а остальные queued callers
освобождаются. Если watchdog выиграл внешний alt, supervisor ещё раз проверяет
reply. При exception закрытие может не иметь reply: результат остаётся Unknown.

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
