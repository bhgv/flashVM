# [GC:] Retry childless last-owner claim loss

Copyright (c) 2026 bhgv

## EN: Preserve the claim result, not a later count sample

The child-packet rollback paths preserve transient last-claim failure. Three
childless returns still exposed the original race: a record/list with no live
mapped owners, an array with zero mapped capacity, and an array whose scanned
fields were all nil. They returned the raw claim result. If an independent
retainer caused that claim to fail and then disappeared, the caller could sample
count==1 and misclassify the supported object as a hard retained refusal.

`claim_childless_leaf` now performs the same atomic claim and explicitly retains
its failure in `retry`. All three returns use this helper after their existing
validation and reader cutoff. No owner, root, field or descriptor is consumed on
failure. The successful path still claims exactly one last reference and uses
the existing physical release. No new admission, lock, packet or world stop is
introduced. Invalid maps and size checks keep their separate refusal behavior.

The preliminary sole-owner check had the same later-sample race. Record/list
and array transfer now share `leaf_has_sole_owner`, which preserves the sampled
non-sole result in retry before returning. This also covers nonempty objects:
no fields are read when that preliminary observation refuses the transfer.

## RU: Отказ захвата не теряется после исчезновения конкурента

Оставались три прямых возврата результата захвата: запись/список без живых
размеченных полей, массив с нулевой ёмкостью карты и массив со всеми nil-полями.
Конкурент мог сорвать захват и снять свою ссылку до следующего чтения счётчика.
Тогда count==1 скрывал временный отказ и объект уходил в остановочный резерв.

Общий helper сохраняет отказ атомарного захвата в retry независимо от позднего
счётчика. Три пути используют его после прежних проверок и эпохи читателей.
Отказ не снимает исходную ссылку/корень и не меняет поля/дескрипторы. Успех
сохраняет прежнее освобождение. Новых блокировок, пакетов или остановок нет.

Предварительная проверка единственного владельца тоже сохраняет наблюдённый
отказ в retry. Общий helper используется записями/списками и массивами; при
неподходящем счётчике он отказывает до чтения полей, включая непустые объекты.

## Actual regression / Настоящая проверка

The wide-owner fixture adds six real allocation shapes: an all-nil mapped
record, a record with no map, a zero-length mapped array, an all-nil nonempty
array, a list without an element descriptor, and a mapped all-nil list. Each
starts with its real sole owner/root after eight observed cutoff waits.

A private diagnostic calls the original atomic primitive after acquiring a real
independent reference and root. It verifies the genuine failed claim at count
two, then removes that owner before returning the unmodified result. The caller
therefore actually observes count one afterward. The normal retry observer
requires a transient result; the object must physically free with zero references
and roots while both Limbo peers remain admitted. No successful claim, collector
response or ownership count is forged.

Four more shapes exercise the preliminary observation: a nil record, zero-length
and all-nil arrays, and a nil list. The diagnostic samples the original count
while an actual independent owner exists, removes that owner, and returns the
unmodified eligibility result. The later caller really sees count one.

Six private omissions remove the claim helper's retry assignment, one per allocation
shape. Each must fail the existing precise retry predicate, rather than passing
through a watchdog, process timeout, compiler error or sanitizer warning. The
four preliminary-read omissions similarly remove its captured retry signal.
The default matrix now contains 31 scenarios in C/JIT and both polling modes: 124
positive applications and 21 semantic omission controls. All prior wide-object,
packet rollback, alias, allocation-pressure and quiescent-assistance cases remain.

The final code passed 98 selected runtime CTest checks. Full-runtime TSan passed
five repetitions of the 124-profile matrix (620 VM applications), 44 graph-claim,
108 module-cycle and 36 Duke applications: 808 positive VM executions without
sanitizer warnings. All 21 omission controls produced their required semantic
errors. Changed handwritten sources are at most 214 lines; upstream is unchanged.
This is scoped validation, not a passing claim for the entire repository suite.

[Channel claim retry](CHANNEL-CLAIM-RETRY.md) subsequently adds queue-locked
refusal capture, four channel shapes and real hosted close delivery. The current
default matrix contains 140 positive applications and 28 semantic omissions.

General stopped tracing, unknown callbacks, specialized native claims, hard graph
limits and stopped shutdown remain. Execution coverage is Linux x86-64; emitted
JIT instructions are outside C TSan instrumentation. ARM64/RV64 execution is not
established by these checks.
