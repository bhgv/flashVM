# [GC:] Preserve ordinary module claim observations

Copyright (c) 2026 bhgv

## EN: Close the remaining preliminary and empty-MP returns

Ordinary Modlink transfer still tested its count directly and returned a raw
last-claim result for `MP == H`. A short-lived independent owner could defeat
either observation and disappear before Fast's later count sample. The module
then looked like a hard retained refusal despite being eligible for shared unload.

The preliminary path now uses `leaf_has_sole_owner`; the empty-MP return uses
`claim_childless_leaf`. Both preserve the original refusal in retry without
changing the Module owner, MP, source reference or root. Closed RC and unsupported
native modules still refuse before these helpers. The mapped-MP packet path keeps
its existing publication-before-claim and complete rollback protocol. No new
world admission, lock, allocation or callback authorization is introduced.

## RU: Повтор не зависит от исчезнувшей ссылки

У обычного Modlink оставались прямое предварительное чтение счётчика и прямой
возврат результата захвата при пустом MP. Кратковременная независимая ссылка
могла сорвать проверку и исчезнуть до следующего чтения Fast. Теперь оба пути
используют общие helper-ы, сохраняющие исходный отказ в retry. Владение Module,
MP, ссылкой и корнем источника не меняется. Закрытый RC и чужие native-модули
остаются отдельными жёсткими отказами. Новых остановок, блокировок или разрешений
произвольных callbacks нет.

## Real regression / Настоящая проверка

The existing module fixture adds three profiles in C/JIT and both polls:
an actual failed empty-MP claim, a failed preliminary sample for an empty module,
and the same preliminary failure for a mapped module. Real references and roots
create the competing owner, actual primitives observe it, and the owner is
withdrawn before returning the original result. A diagnostic requires the captured
retry before routing. Native metadata, MP clones/originals, strings, Type owners,
reader cutoffs and physical frees remain production operations.

Three new omission controls remove the appropriate retry assignment. Together
with the original profiles this gives 36 positive applications and seven semantic
controls. Missing injections fail their own evidence checks; watchdogs, compiler
errors, signals and sanitizer reports do not pass an omission control.

The module semantic controls run in the ordinary CTest build. Module TSan runs
use `--positive-only`, as their normal lifecycle joins the external reader.
Running the old intentionally fatal control 5 under TSan reports a finished,
unjoined diagnostic reader when its expected semantic error terminates early;
that run is a failure and is not counted as sanitizer acceptance. No sanitizer
warning is suppressed. The production VM and positive observer lifecycle join
normally. This artifact of the fatal harness remains outside the claimed TSan set.

## Owner-path audit / Проверка путей владения

| Supported transfer | Preliminary count | Childless claim | Staged child claim |
| --- | --- | --- | --- |
| Record/list | Captured sole-owner helper | Captured claim helper | Explicit retry after rollback |
| Array/view | Captured sole-owner helper | Both empty returns use the claim helper | Retry preserved for rooted claim loss |
| Ordinary module | Captured sole-owner helper, now used here | Empty MP uses the claim helper, now used here | Existing MP roots/reservation rollback and retry |
| Quiet channel | Queue-locked native observation | Captured refusal survives later snapshots | Captured refusal survives buffer-packet rollback |

This audit concerns supported leaf transfer, not every reference-count operation.
Final validation passed 98 selected runtime CTest checks, including all 32
wide/channel and seven module semantic controls in the ordinary build. Full-runtime
TSan passed five 156-profile repetitions (780 VM applications), 36 ordinary-module,
44 graph-claim, 108 module-cycle and 36 Duke applications: 1,004 positive executions
without warnings. The first wide/channel run also passed its 32 TSan omission
controls. Module TSan acceptance uses positive-only runs as described above.
Changed handwritten sources are at most 316 lines; upstream remains unchanged.
These results do not claim a passing entire-repository suite.

General stopped tracing, unsupported finalizers, graph limits and stopped shutdown
remain. Execution coverage is Linux x86-64; emitted JIT instructions are outside
C TSan instrumentation. ARM64/RV64 execution is not established here.
