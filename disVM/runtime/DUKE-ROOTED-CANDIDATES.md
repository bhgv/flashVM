# [GC:] Duke queue owners during large packet replay

Copyright (c) 2026 bhgv

## EN: Consolidate owners while retaining the seed

A refused Fast snapshot can replay as a packet containing an owner for every
node. Its release publishes many independent cycle candidates. In the 1,040-node
two-slot ring, Cycles routes these candidates to Duke. Every Duke capture then
sees other queue roots as external owners: repeated complete captures retire one
candidate at a time. The original ten-second TSan watchdog failed after thousands
of passes, despite a committed replay and preserved owners.

After each suspended Duke discovery slice, the sole consumer now inspects a
bounded queue snapshot. It preserves the current seed and removes other queue
owners only when the allocation has another registered root and an atomic
nonfinal reference drop succeeds. This is the existing independently-rooted
candidate rule, applied while the in-flight capture retains its discovery pins.
An indexed key alone never authorizes a reference drop. Pin ownership, abandoned
pin rescue and physical retirement retain their existing protocols.

The queue transfer mutex excludes appenders during the FIFO rotation. Every
unconsumed entry returns in its original order with the original seed at the
head. At most 4,096 ready entries are inspected; a larger queue keeps its existing
single-candidate behavior. The sweep allocates no memory and invokes no managed
destructor. This local metadata mutex does not acquire exclusive world admission
or park guest threads. No claim of size-independent throughput follows from this
bounded optimization.

Removing queue owners invalidates root observations already recorded in the
capture. A `roots_pruned` flag survives suspension and storage growth. Before
classification, Duke reacquires and checks captured fields, releases connected
pins through the existing confirmation protocol, and requests a fresh capture
without withdrawing the seed. If fields changed or guards are busy, the ordinary
abandoned-pin protocol preserves their ownership instead. Treating the old live
marks as current could drop the last candidate and strand an unreachable ring;
the restart is an ownership requirement, not only a performance choice.

Cycles can also consume independently rooted candidates while Duke is suspended.
Consequently the private classification module resets derived reachability and
its dense work arrays, and samples current roots again. Intrinsic unsupported
sources keep a separate bit. Before consuming a live Duke seed, the collector
reclassifies once more after connected pins were released, with all captured
fields guarded and candidate-owner removal excluded through seed consumption.
The same transfer mutex now covers other consumers' nonfinal candidate drops.
If fresh classification no longer proves the seed live, its owner is preserved
for a new capture. This closes the separate race seen during repeated replay
stress, in which Cycles had consumed the last other queue roots.

## RU: Снять лишних владельцев, сохранить исходный seed

Возврат большого снимка создавал волну отдельных кандидатов. Каждый полный обход
Duke видел корни остальных записей и снимал лишь одного владельца. Теперь между
порциями обхода снимаются лишние владельцы очереди с независимыми корнями, пока
узлы удерживаются снимком. Исходный seed и порядок оставшихся записей сохраняются.
Наличие ключа не служит доказательством владения; финальные ссылки не снимаются.

После снятия владельцев прежние наблюдения корней недействительны. Duke сверяет
поля, освобождает связанные удержания обычным протоколом и повторяет обход,
сохранив seed. При конфликте guards или изменении полей действует передача
отменённых удержаний. Пропуск повторного обхода может потерять последний seed.
Локальный мьютекс очереди не останавливает гостей. Очереди свыше 4,096 записей
сохраняют прежний путь; общей гарантии времени для произвольных графов нет.

Cycles тоже может снять соседние корни во время приостановленного обхода Duke.
Поэтому вычисленные live-метки и рабочие массивы сбрасываются, корни проверяются
заново. Перед снятием живого seed выполняется ещё одна классификация под guards
всех полей и с исключением снятия владельцев другими читателями до потребления
seed. Если свежая проверка больше не доказывает живость, seed сохраняется.
Отдельный бит сохраняет неподдержанные источники при пересчёте достижимости.

## Acceptance / Приёмка

The default native claim fixture now includes both original 1,040-node replay
cases, alongside the smaller protocol cases: eleven scenarios, each interpreted
and JIT-compiled with both polling modes. FULL/BYTES pressure still lasts eight
actual Fast responses. A real guarded field move must stale the original
snapshot; a real packet commit, fresh claim, final reader epochs and exactly-once
physical frees must all complete while both native peers stay admitted. The
unsupported final owner must remain rooted until peers leave. Neither internal
watchdog nor peer admission nor physical-free predicates were relaxed.

A ninth semantic omission control bypasses the restart after actual queue-owner
removal. A private observer at the real classification boundary rejects stale
root observations with a specific semantic error. A missing-seed watchdog,
process timeout, signal, sanitizer report or build failure cannot pass this
control. `--large-replay` remains available to run the eight large replay profiles.

ThreadSanitizer validation passed 30 consecutive repetitions of those eight
large replay profiles (240 VM applications), followed by the full 44-profile
claim matrix, 108 module-cycle applications and 36 Duke pin-retirement
applications: 428 positive VM executions without sanitizer warnings. The nine
claim omission controls also produced their required semantic errors. The final
ordinary acceptance passed 97 selected runtime CTest checks. Changed handwritten
sources are at most 422 lines; upstream snapshots are intact. This does not
establish a passing result for the entire repository suite.

General stopped tracing, unsupported callbacks and stopped shutdown remain.
Execution coverage here is Linux x86-64; C TSan does not instrument emitted JIT
instructions. ARM64/RV64 execution is not established by this work.
