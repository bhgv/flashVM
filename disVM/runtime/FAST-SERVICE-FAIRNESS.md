# Independent Fast services / Независимые службы Fast

Copyright (c) 2026 bhgv

Fast contains three services: managed-ticket handoff, emergency-string retirement,
and RC reclamation (owners, leaves, graph retries and frame packets). Their reader
cutoffs are independent. An old RC owner can be safe to reclaim while a later
reader still protects ticket and reserve payloads.

The previous coordinator always rounded the ticket share upward, then rounded
half of the remaining budget upward for reserves. Any nonzero result charged the
whole share, including a reader retry. With budget 1 a blocked ticket could consume
every pass; with budget 2 blocked tickets and reserves could leave RC permanently
at zero. A reserve alone could also starve RC at budget 1. The default budget 256
was not subject to this particular zero-share starvation.

`gc/fast/service_budget.inc` now samples native readiness for all three services,
divides the budget among nonempty classes and rotates the remainder. A blocked
reader consumes only its class's allowance. A continuously ready class receives
an attempt within at most three nonzero passes even at budget 1. Empty classes
receive zero; the total allowance never exceeds the caller's budget. Services
with zero allowance are not entered. Later publication still wakes Fast normally.
The sole Fast consumer owns the cursor; no allocation, new exclusion or tracing
fallback is introduced. RC keeps its separate inner queue scheduler.

У Fast три службы с независимыми эпохами: передача managed tickets, освобождение
резервных строк и RC. Раньше округление квоты tickets, затем reserves могло при
бюджете 1/2 оставлять готовый RC без единой попытки. Ожидание читателя считалось
занятой квотой, поэтому ошибка повторялась бесконечно. При штатном бюджете 256
этого конкретного голодания нулевой квоты не было.

Теперь бюджет делится между непустыми службами, а остаток выдаётся по кругу.
При бюджете один каждая постоянно готовая служба получает попытку не позднее
третьего ненулевого прохода. Ожидание одной службы не забирает чужую квоту.
Новых блокировок исполнителей, выделений памяти или резервной трассировки нет.

## Actual ownership evidence

`test_fast_services.py` uses genuine native guest processes, Fast callbacks,
managed tickets, emergency reserves, registry roots and physical `poolfree`.
No work callback is replaced with synthetic progress. A gate only orders setup,
and the wrapper selects pass budgets 1, 2, 3 and 256.

The parent publishes an RC string before another OS thread begins a raw borrow.
It then retires a managed ticket and/or its actual emergency reserve after that
borrow begins. The reader touches those later payloads and keeps the epoch open.
Fast must physically reclaim the older RC string while the later services still
hold their references and roots. Two genuine guest sleep frames remain admitted
throughout, advancing epochs alternately; global quiescence cannot explain the
free. The parent resumes with the same context, Prog and live register storage.

Three queue combinations (both, tickets only, reserves only), four budgets, C/JIT
and both polling configurations give 48 positive profiles. The restored original
coordinator must fail at the exact bounded-attempt starvation invariant in 16
profiles: budget 1/2 with both queues, and budget 1 with either queue alone.
Timeouts, signals, compiler failures and sanitizer diagnostics never count as
successful detection. `--positive-only` omits intentional process failures.

Тест удерживает настоящие поздние ticket/reserve под borrow и требует физического
освобождения более старого RC при двух допущенных гостях. Контрольные варианты
возвращают прежний координатор и обязаны дать конкретный отказ по числу попыток.
Положительный прогон проверяет 48 сочетаний; отрицательный — 16. Результаты
Linux x86-64 не подтверждают полную реализацию ARM64/RV64 или инструментирование
машинного кода JIT санитайзером.

Validation: selected hosted native audit 32/32. ThreadSanitizer passed all 48
new positive profiles plus 12 managed-retirement profiles. The 16 restored
legacy controls passed their exact failure checks in the ordinary build.
Upstream snapshots are unchanged; changed handwritten sources are at most
347 lines.
