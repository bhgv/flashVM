Current native behavior is documented in [Concurrent large graphs](CONCURRENT-LARGE-GRAPHS.md).
The hard-limit/retained routing described below is historical.

# Duke hard refusals preserve progress and eventually leave shared rescue

Two different cases need different handling:

| Duke result | Next action |
| --- | --- |
| Suspended valid discovery slice | Keep its current queue head and scratch for resumption |
| Transient capture refusal | Preserve owner/root and rotate for another bounded attempt |
| Hard refusal, fallback reservation fails | Preserve owner/root and rotate so another graph runs |
| Growing snapshot exceeds its hard limit | Let Duke dispatch move the seed to `cycle_refused` |

Previously a failed fallback reservation kept the rejected candidate at the head.
Duke repeatedly tried that same graph, starving independent candidates behind it.
The dispatcher now rotates this unchanged entry after a failed move. Successful
suspended slices still retain their head; no discovery continuation is migrated.

The second defect was specific to oversized growing captures. The scanner moved
such seeds to the mandatory release FIFO. Fast's mandatory-owner rescue then sent
them to Cycles, which sent them back to Duke. Lifting queue pressure could leave
that same uncollectable shared work cycling indefinitely. The scanner now reports
a hard refusal for an oversized Duke capture; the existing Duke dispatcher owns
its fallback handoff to `cycle_refused`. Fast cannot republish that queue's seed
as mandatory rescue work. Under retained admission it removes the queue root and
releases the queued reference, allowing subsequent quiescent tracing to collect
the self-cycle.

Every failed transfer preserves its source reference, registered root and queue
entry. Allocation/token/queue algorithms are unchanged. This introduces no world
stop, execution mutex, Prog migration or live-register restoration. Graphs beyond
the hard snapshot bound still require conservative tracing; the fix guarantees
that they neither block supported neighbours nor circulate forever as shared
rescue work.

## Real runtime regression

`disvm_duke_queue_progress` runs the real Limbo/native runtime with two permanently
bound peers. They alternate actual reader epochs while at least one stays
admitted. The parent creates two genuine mapped arrays, each with one self-edge
and otherwise valid nil fields:

- 70,000 fields exceed the real 4 MiB Duke snapshot storage bound.
- 2,300 fields exceed ordinary Cycles but fit growing Duke discovery.

The fixture observes the real Duke callback and injects FULL or BYTES only at its
actual fallback reservation for the oversized head. No capture result, root,
reference count or reader epoch is fabricated. It first requires repeated real
reservation failures, then publishes the smaller cycle. That cycle must physically
free under shared admission while queue pressure and admitted peers remain.
The oversized array must retain exactly its self-edge, queued reference and one
registered root. Its fallback destination must be the actual queue supplied to
Duke, rather than the mandatory rescue FIFO.

After pressure/peer release the fixture waits for actual service completion,
requests genuine quiescent color rotations after source-root withdrawal, and
requires both arrays to physically free once. Allocation generations prevent
reused addresses from satisfying the observations. The element Type creator must
retain exactly its one independent descriptor reference afterward.

Eight positive profiles cover FULL/BYTES, C/JIT and both polling paths. Two private
semantic controls remove failed-move rotation or restore oversized-to-mandatory
routing. Each must produce its precise protocol diagnostic; compiler failures,
signals, process timeouts and sanitizer warnings do not count as detection.
TSan covers C/host/fixture code, not emitted JIT instructions. Linux x86-64 is the
execution evidence; no ARM64/RV64 execution is certified by these checks.


## Validation boundary on this host

The normal validation build uses `-O1 -g`; TSan uses `-O1 -g -fsanitize=thread`
with frame pointers. Completed sanitizer suites cover 380 positive VM runs and
the two new semantic omissions. The new runtime regression and neighbouring
ownership tests pass. Of 100 selected CTest checks, 97 pass after isolated
revalidation of the timing-sensitive Fast root-pruning omission. Three aggregate
GC suites (`native_cycle_gc`, `dedicated_cycle_gc`, `idle_cycle_gc`) exceed their
existing 180/180/130 second deadlines in the parallel run on this host. Their
limits and application assertions were not relaxed. This is not a fully green
100-test parallel acceptance run.

One offending full native/JIT application profile was also compared using private
relinks that restore both production changes to the `a06a8af` base. The base and
new runtime both completed successfully, in approximately 38 and 42 seconds;
both reported zero Duke passes. This comparison isolates that application from
the changed Duke paths, but does not certify all three timed-out aggregate suites.

# Прогресс Duke при жёстких отказах

Отказ резерва больше не удерживает неподдержанный граф в голове Duke: ссылка,
корень и запись сохраняются, а независимый сосед получает следующий проход.
Незавершённый допустимый снимок по-прежнему возобновляется на своей голове.

Также устранён круг для снимков за жёстким пределом: Duke → обязательный FIFO →
Fast → Cycles → Duke. Теперь такой отказ передаётся диспетчеру Duke и затем
очереди `cycle_refused`, которую обрабатывает резервная очистка. Слишком большой
граф всё ещё требует трассировки в покое; исправление не выдаёт этот путь за
полностью конкурентную сборку.

Проверка использует настоящие циклические массивы на 70 000 и 2 300 полей,
два закреплённых потока, реальные FULL/BYTES на границе резерва, корни, эпохи и
физические free. Меньший граф освобождается при активных соседях; большой —
после снятия давления и настоящей трассировки. Два намеренных нарушения
обнаруживаются точными семантическими ошибками.
