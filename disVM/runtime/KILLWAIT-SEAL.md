# Registered-child retirement / Ожидание registered-child

Copyright (c) 2026 bhgv

The first 41-workload RV64 run passed the new owner-monitor probe but failed the
error-isolation workload. The parent's first visible failure was its unexpected
killwait result; shutdown then cancelled other residents' native sleeps. Their
combined sleep/error assertion consequently reported replaced error text. This
does not justify weakening either the killwait or error-isolation assertions.

`prepare_dedicated_child_wait` previously sampled a registered child's lifetime
twice. The registry mutex protects the child's registration/storage, but the
child seals its lifetime with an owner-local CAS before taking that mutex to
withdraw the root. The following order is therefore valid:

1. Parent finds the registered child and samples a live lifetime.
2. Child seals itself, while still registered.
3. Parent's second admission check sees the seal and skips preparing the wait.
4. The real result function still finds the registered child and returns
   `NotWaiting`, despite killwait's retirement contract.

Registration now admits the wait. One lifetime sample only decides whether an
exit request is necessary; an already sealed child still receives the matching
retirement notification. If sealing follows a live sample, registration keeps
the endpoint valid until the existing mutex is released. Missing/foreign child
handling is unchanged. There is no new lock, foreign register read, process
migration or restoration of live registers from `Prog.R`.

`test_killwait_seal.py` forces this exact order with real parent/child owners.
The child reaches its real retirement call, waits for the parent's actual live
sample, then performs its own production CAS. Positive profiles must park and
return `Retired`. A private control restores the obsolete second check and keeps
the sealed child registered through the real result call: it must return exactly
`NotWaiting` and exit 12 with the specific diagnostic. Compile failure, sanitizer
warning, timeout, signal or a missing-child `Denied` is never a passing control.
C/JIT and both polling modes run in normal and TSan builds. The architecture
manifest now includes this as its forty-second positive workload.

Первый RV64-набор из 41 нагрузки прошёл новые owner-local hooks, но обнаружил
отказ в error-isolation. Сначала родитель получил неверный результат killwait;
остановка затем отменила sleep остальных потоков. Их объединённая проверка
sleep/error сообщила о замене текста. Ослаблять эти проверки нельзя.

Mutex реестра защищает регистрацию и память ребёнка, но его владелец закрывает
lifetime атомарным CAS до снятия root. Два чтения родителя могли увидеть сначала
живое, затем sealed-состояние ещё зарегистрированного ребёнка. Старый допуск
пропускал wait, а настоящий result возвращал NotWaiting.

Теперь наличие ребёнка в реестре разрешает ожидание. Единственный lifetime-снимок
лишь исключает лишний запрос выхода; sealed-child также должен дать квитанцию
retirement. При sealing после живого снимка реестр сохраняет endpoint до снятия
существующего mutex. Новый замок/эксклюзив и восстановление Prog.R не добавлены.

Регрессия принудительно совмещает настоящее чтение родителя и настоящий CAS
ребёнка на его постоянном потоке. Positive обязан ждать и вернуть Retired;
контроль со старым чтением обязан вернуть именно NotWaiting и код 12. Ошибки
сборки/санитайзера, таймаут, сигнал и Denied не засчитываются. Wide включает
42-ю нагрузку; архитектурный результат фиксируется только после исполнения.


## Verified regression / Проверенная регрессия

The final [workflow 43355415904](https://github.com/bhgv/flashVM/actions/runs/43355415904)
on `1c8c7ead36370c5927490c02744d3ec0475d9ecd` passed all 42 workloads in native
AMD64, native ARM64, AMD64 TSan and emulated RV64 jobs. Both the forced sealing
probe and the original resident-errors workload passed on every target. Local
normal/TSan runs additionally restored the old admission: all four C/JIT/poll
controls returned the actual NotWaiting result and the exact exit-12 diagnostic;
all four corrected profiles parked and returned Retired.

Финальный workflow указанного коммита прошёл 42/42 на четырёх профилях.
Принудительный sealing и исходный resident-errors прошли на каждой цели.
Локальные обычная/TSan-сборки также выполнили все четыре старых контроля с
настоящим NotWaiting/кодом 12 и четыре corrected-профиля с ожиданием/Retired.
