# Large cyclic graphs without collector exclusion

Copyright (c) 2026 bhgv

## Production fixes

Duke no longer applies the old 4 MiB discovery-buffer policy ceiling. The former
70000-field failure now completes through shared discovery, guarded confirmation
and reclamation while two permanent native guests remain admitted. The native
world's prohibition of ordinary and quiescent collector stops remains enabled.
No retained callback or general heap trace is introduced.

The field bound is derived from the actual discovery layout:

- per node: two pointers, two unsigned counters and one byte;
- per field: one pointer and ten unsigned counters, including both hash tables;
- total storage must fit `PTRDIFF_MAX`;
- four hash slots per field must fit the existing unsigned index representation.

Capacity grows geometrically up to that representable bound. This is not a larger
fixed policy limit. Allocation failure still preserves the original buffers,
indices, cursors, roots and reference owners, returning the existing busy retry.
Actual representation exhaustion remains an explicit refusal under strict policy.
This does not promise allocation success under persistent native memory exhaustion.

A second defect was found in `dvm_rc_reserve_record_fields`: when `edges_used`
exceeded `field_limit`, unsigned subtraction could make the remaining capacity
appear enormous and accept more fields. Extents are now validated before that
subtraction; additional fields must fit the native bound minus used fields before
forming their sum. Those invalid extents are rejected without touching storage or owners.
The same native limit is used when checking repeated mapped array fields.

## Actual concurrent regressions

The Duke fixture retains real arrays with 70000 and 2300 mapped fields. It refuses
only the selected large graph's actual snapshot `calloc`, either at initial
allocation or subsequent growth, until the independent graph physically frees.
The large source must preserve exactly its queued root and existing references.
After pressure clears, it too must physically free with two guests still admitted,
zero roots/count and no stopped world. A dense profile has 70000 actual self-edge
occurrences, not merely nil slots, and checks the genuine count of 70001 owners.

Twelve positive profiles cover sparse/dense graphs, both allocation stages, C/JIT
and both polling paths. One control restores the old 4 MiB bound and must produce
strict `refused-cycle-graph` after independent progress. Another removes the invalid
extent guard and must fail the exact underflow counterexample. Signals, timeouts,
compiler failures and sanitizer diagnostics do not count as successful controls.

The other strict-mode audit failures were separated from obsolete retained-test
scaffolding without relaxing the runtime:

- `disvm_packet_fairness` now uses a known callback-free record and real native
  leaf-metadata allocation refusals. Its packet cursor and suffix retain exact
  roots/counts while independent packets and channel retries progress. Both held
  owners then free before peers leave. Thirty-two profiles preserve budgets one,
  two and default, channel claim races, zero-work returns and final shared cleanup.
  Three omission controls remove rotation, invent progress or ignore the allocator
  failure; each must fail its own semantic predicate.
- `disvm_fast_claim_storage` derives its private fixture without the deliberately
  unreviewed callback. Forty-four profiles retain actual 0/1/513/514/520 Module counts,
  vector allocation failures, stale field changes, FULL/BYTES snapshot replay,
  final reader epochs and real physical frees. The five applicable ownership and
  classification omission controls remain. A paired pruning probe publishes one
  genuine extra queue owner for an already pinned node under the real transfer
  mutex. Production pruning removes it, forcing stale-root observations to restart.
  The positive case must finish; removal of the restart guard must fail at the
  actual classification boundary. Fixture publication is counted like other real
  owners, preserving the enqueued/released equality. Old retained-assistance/return-work
  controls 10/12/14/16 are not applicable to the closed concurrent backend.
- Module omission control 4 intentionally makes ordinary module finalization
  unreviewed. It now requires the strict status-70 rejection instead of expecting
  eventual stopped cleanup. Other module probes and omission controls remain.

The original legacy fixture sources remain available for source comparison.
Unknown native callbacks remain unsupported and are still tested by the strict
production guards. They were not registered as safe or executed concurrently.
No test disables the native world's stop prohibition.

## Русский

У Duke убран искусственный предел снимка в 4 МБ. Размер ограничен адресуемой
памятью и реальным диапазоном unsigned-индексов. Граф на 70000 полей теперь
освобождается при двух работающих гостях без эксклюзива. Проверен также вариант
с 70000 настоящих ссылок на себя и реальным счётчиком 70001.

Исправлено беззнаковое вычитание: использованные рёбра сверх ёмкости могли
превращаться в огромное «свободное место». Теперь размеры проверяются до вычитания
и сложения. Отказ выделения памяти остаётся обратимым повтором с целыми корнями,
ссылками и буферами; прежний остановочный путь не вызывается.

Проверки пакетов и захватов графов теперь используют реальные временные отказы
памяти вместо намеренно неизвестного финализатора. Сохранены настоящие free,
проверки счётчиков/корней/эпох, захватов и replay, а также подходящие контроли
пропусков. Неизвестные native callbacks по-прежнему дают явную ошибку: доверие
к ним и эксклюзив не включаются ради зелёных тестов.

Linux x64 validation does not establish ARM64/RV64 JIT coverage or prove that all
native callbacks are thread-safe. Native representation and memory bounds remain.


## Validation on Linux x64

- The same 29-test strict audit is satisfied after targeted revalidation: the
  complete run passed 28 tests and exposed nondeterministic setup in pruning
  control 19. The paired real-owner publication probe then passed the complete
  claim matrix, including the control. The other 28 tests were not disabled.
- Two additional checks, `disvm_rc_roles` and `disvm_large_record_cycle`, passed.
- TSan passed 88 positive profiles: 12 Duke growth/alias cases, 32 concurrent
  packet cases and 44 claim/replay/pruning cases. Fatal negative controls ran
  normally; emitted JIT instructions are not C-TSan instrumented.
- The first attempted audit was stopped after configuration invalidated generated
  headers; the reported complete audit ran after a finished rebuild.
- Largest changed handwritten source: 423 lines. Upstream is unchanged.
