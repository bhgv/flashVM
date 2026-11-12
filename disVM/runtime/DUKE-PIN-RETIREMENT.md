# [GC:] Duke abandonment preserves cycle candidates

Copyright (c) 2026 bhgv

## EN: Ownership and failure

A mutator can detach an edge after Duke has pinned its destination. Cycles may
consume the real old-edge retirement seed while Duke's discovery root still
makes the detached cycle live. Previously, abandoning that stale discovery
simply dropped its nonfinal pins. The detached cycle then had only internal
references and no remaining candidate; reclaiming it required stopped tracing.

Before abandonment, Duke reacquires the captured field guards and checks every
recorded edge. If unchanged, its queued source still reaches each pinned target,
so those pins can be dropped directly under guards. Changed fields or guard
contention leave pins intact for rooted retirement. This avoids repeatedly
publishing whole unchanged large graphs.

`gc/duke/pin_release.inc` transfers remaining nonfinal candidate discovery pins
to the mandatory retirement FIFO. The same strong reference and individual root
remain attached to the object: no additional retain, root allocation or early
root withdrawal occurs. Successful publication consumes the journal entry.
After commit Duke touches only native counters and its journal cursor, because
another collector may already reclaim the published object.

FIFO refusal leaves the cursor, reference and root unchanged for another shared
Duke pass. This path does not set the cleanup fallback flag or request a stopped
Fast pass. Existing final-owner, noncandidate, closed and stopped cleanup routes
remain. `NATIVEDUKEPINS` reports successful seed transfers and refused reservations;
successful transfers also participate in accepted/released queue accounting.

## RU: Владение и отказ

При отмене устаревшего обхода недостаточно снять непоследний pin. Настоящий
кандидат от удалённого поля уже мог пройти Cycles и остаться живым из-за корня
Duke. Теперь существующие ссылка и корень переходят в обязательную очередь,
поэтому отделённый цикл снова попадёт на проверку после снятия остальных pin.
При неизменных рёбрах pin снимаются под guards: связь с исходным seed сохранена.
Если guards заняты, pin остаются для передачи с корнем.
Отказ очереди сохраняет запись журнала целиком для следующего shared-прохода,
не запрашивая общую остановку. После успешного commit объект больше не читается.

## EN: Actual runtime regression

`test_duke_pin_retirement.py` runs a real Limbo resident application with two
permanently admitted native peers. Its native fixture creates 2,366 actual typed
managed allocations: a self-cyclic source, a 64-record branch ring, a pointer
array and 2,300 string leaves. Discovery exceeds the initial field budget and
resumes through Duke. After the real discovery guards end, a test callback takes
an independent source reference/root, detaches its branch under real write
guards and calls production `destroy` on the old field owner. The callback runs
on Duke's thread; it is not an independently scheduled guest mutator.

The callback waits, without field guards or registry locks, for actual Cycles to
consume that old-edge seed. Duke must then reject its stale count observation
and abandon discovery. Count rejection retains its existing busy/retry result.
The detached ring must physically free while the independent source remains
rooted. After that owner is released, the source, array and every string must
also physically free, exactly once by allocation generation, with two admitted
peers and no stopped Fast admission while target objects remain.

Both normal and pressure scenarios run in C and JIT with both poll modes.
Pressure refuses the actual mandatory reservation until eight real Duke retry
responses occur. Four private omission controls require exact semantic failures:
dropping the candidate pin, removing its root before commit, bypassing guarded
field comparison before releasing connected pins, or advancing the journal
cursor on refusal. Compiler errors, signals, sanitizer reports and process
timeouts do not count as controls.

## RU: Проверка настоящего освобождения

Тест сначала требует физического освобождения отделённого кольца при живом
внешнем владельце source, затем всех 2,366 объектов при работающих соседях.
Проверяются идентичность поколения, отсутствие двойного free, баланс очереди,
сохранение корня до commit и курсора при восьми отказах. Четыре намеренно
сломанные частные сборки должны завершиться конкретной семантической ошибкой.

## Validation / Проверки

The final Linux x86-64 tree passed 95 selected CTest checks, including the large
record cycle and the new four-control regression. Full-runtime ThreadSanitizer
builds passed 160 positive pin-retirement applications (20 repetitions of eight
profiles) and 108 positive module-cycle applications without sanitizer warnings.
All changed handwritten source files remain at most 500 lines. These are scoped
checks, not a claim that the entire repository test suite passes.

## Remaining scope / Оставшиеся ограничения

The bounded Duke snapshot size, including its 4 MiB ceiling, remains. This change
does not remove general stopped tracing, unsupported native finalizers or stopped
shutdown. Runtime validation is Linux x86-64; generated JIT instructions are not
instrumented by C ThreadSanitizer. ARM64 and RV64 execution are not established
by these checks.
