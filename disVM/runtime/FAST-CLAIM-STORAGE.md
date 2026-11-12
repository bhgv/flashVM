# [GC:] Native ownership vector sizing and shared claim retry

Copyright (c) 2026 bhgv

## EN: Actual native cardinality

Fast previously sized its temporary native Module vector by the number of all
managed nodes in the snapshot. A large pure record graph allocated the vector
despite having no native owners, and a large sparse graph allocated space for
unrelated records. Allocation failure returned the same boolean refusal as a
stale graph, causing packet replay instead of retaining the original snapshot.

`module_graph_claim.inc` now counts the actual native entries attributed to dead
Modlinks before selecting storage. Each unique Module contributes its existing
aggregated owner count once; managed graph size no longer determines native
vector size. Up to 513 native entries use the existing local arrays. Larger
vectors allocate exactly the native entry count, before the Module registry mutex.

The private helper now returns the existing cell result type. Native vector
allocation refusal is BUSY, with all managed/native counts and roots untouched.
`dvm_gc_fast_confirm_graph` propagates that result and releases its field
reservation. Fast keeps the same rooted snapshot for another confirmation.
Ownership/count/field/epoch refusals remain REFUSED and retain their existing
rollback/replay policy. Success still atomically claims managed and native owners,
unlinks Modules and waits for the final reader epoch before physical reclamation.

## RU: Размер и повтор захвата

Размер временного вектора теперь определяется числом настоящих native-записей,
а не всех узлов графа. Большой граф без Module или с одним Module больше не
требует heap-вектора. Нехватка памяти большого вектора возвращает BUSY: прежние
счётчики и корни сохраняются, резервации полей снимаются, Fast повторяет захват
того же снимка. Настоящий отказ владения, полей или эпохи по-прежнему означает
REFUSED с прежним откатом/повтором пакета.

## EN: Actual graph acceptance

`test_fast_claim_storage.py` drives a real Limbo resident application with two
permanently admitted native peers. Its native fixture constructs 1,040 actual
typed managed allocations and independent ordinary Modules using production
factories, original MP owners, Modlinks, field retains and one real seed ticket.
No managed heap or successful collector result is substituted.

| Actual Modules | Managed nodes | Required native vector storage |
| --- | --- | --- |
| 0 | 1,040 | Local, no heap allocation |
| 1 | 1,040 | Local, no heap allocation |
| 513 | 1,040 | Exact local-capacity boundary |
| 514 | 1,040 | Dynamic, exactly 514 entries |
| 520 | 1,040 | Dynamic, exactly 520 entries |

Every Module has a distinct registry identity and exactly one graph-owned native
owner. Its original MP and instance MP share the same allocation with two real
physical owners. Remaining nodes are ordinary cyclic records. Each scenario
executes in C and JIT with both poll modes: 20 applications per driver run.

For each dynamic case, a wrapper refuses eight actual vector `malloc` calls.
Every refusal must return BUSY on the same snapshot, preserving every managed
count/root and every native Module count, without requesting a final epoch.
The next allocation must succeed and its native buffer must reach real `free`
before confirmation returns. Local/sparse cases must perform no vector malloc.

After actual successful claims, peer epochs stay old for eight real Fast retry
responses. No managed allocation or native Module may physically free during
that interval. After readers advance, all 1,040 managed allocations and every
native Module must free with admitted peers. Allocation generations detect reuse,
registry lookups must fail before Module metadata free, and the Type creator must
remain its sole final owner. Accepted/released queue accounting must balance.

Four private omission controls restore node-based allocation, misclassify OOM as
REFUSED, size a dynamic vector by graph nodes, or omit its free. Each must fail
its exact semantic predicate; compiler errors, signals, process timeouts and
sanitizer reports never count as successful controls.

## RU: Проверки

Проверяются настоящие графы с 0, 1, 513, 514 и 520 независимыми Module.
На динамических векторах восемь отказов malloc должны сохранять исходный снимок,
все ссылки, корни и native-счётчики. После восстановления памяти проверяются
настоящий захват, ожидание последних эпох, физическое освобождение объектов,
снятие Module из реестра и баланс очередей при работающих соседях.
Четыре намеренно сломанные частные сборки проверяют границы протокола.

## Validation / Проверки

The final behavior passed 97 selected runtime CTest checks, including all four
new semantic omission controls. Full-runtime ThreadSanitizer runs passed 400
new graph applications (20 repetitions of 20 profiles), 108 existing module-cycle
applications and 36 existing Duke applications: 544 VM executions without
sanitizer warnings. Dynamic cases exercised 1,280 actual vector malloc refusals.
Changed handwritten sources remain at most 268 lines. This is scoped validation,
not a claim that the entire repository suite passes.

## Remaining scope / Оставшиеся ограничения

This retains the existing bounded graph/reservation scheme; it is not removal
of general stopped tracing, unsupported native finalizers or stopped shutdown.
The peers prove admitted execution during reclamation, not per-operation latency
for conflicting field accesses. Runtime execution here targets Linux x86-64;
generated JIT instructions are outside C ThreadSanitizer instrumentation.
ARM64/RV64 execution is not established by these checks.
