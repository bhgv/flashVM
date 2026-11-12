# Independent Fast queue budgets

Copyright (c) 2026 bhgv

Native execution now follows [strict concurrent policy](STRICT-NATIVE-CONCURRENCY.md).
Retained-backlog behavior below records the earlier implementation and its tests.


## English

Fast previously reserved a rounded-up third of its budget for mandatory owners,
then spent the remainder on retry/new leaves before visiting frame packets. With
budget one, a single unsupported mandatory owner could consume every pass without
making progress. Without that owner, a repeatedly refused leaf could spend the
only unit before ordinary leaves or frames were reached. A sustained leaf stream
could also consume all work available to frame packets at larger budgets.

The shared callback now divides its visit allowance between four nonempty native
queues: mandatory owners, retry leaves/graphs, new leaves/graphs and frame packets.
Remainder units rotate after their last recipient. Empty queues receive no share.
Thus a persistently nonempty category gets a turn even at budget one; a refused
claim cannot consume another category's allowance. Queues published after the
metadata sample are considered on a subsequent pass. These are brief metadata
observations, not a new global execution mutex.

`work_budget.inc` owns only the sole Fast consumer's rotation cursor.
`leaf_release.inc` processes one independently budgeted leaf queue. Every visited
head consumes one unit, including an unelapsed epoch, failed last-owner claim,
failed transfer or refused graph. Visits also stop at the queue's initial ready
count. A graph claim/reclamation still completes its indivisible operation and
charges its additional size; this is not a hard real-time bound on graph work.
`shared_release.inc` composes these steps under the existing shared reader lease.
All fragments stay in the existing RC translation unit.

Root publication, reader cutoffs, last-count claims, guarded graph confirmation
and supported finalization remain unchanged. Refused owners retain their roots,
counts and packet cursors. A queue rotation or unused share is not reported as
completed work. Unknown finalizers continue through genuine quiescent cleanup;
removing them or abandoning their owners would hide the progress defect.

The real VM fixture keeps two permanent resident peers admitted while it holds:

- an unsupported finalizer in mandatory retirement;
- an unsupported finalizer and suffix in a frame packet;
- ordinary leaf and independent frame owners;
- one or two channels with actual temporary strong references and registry roots
  installed before the real observed claim and removed immediately afterward.

The channels must remain retryable while independent queues free their real
allocations. A bounded profile holds those actual races for sixteen observations
and checks the number of claim visits per Fast call. Peers leave only after exact
root/count and physical-free checks; both unknown finalizers then run exactly once.
C/JIT and both polling paths are covered with budgets one, two and the default.
Private semantic controls restore mandatory/retry monopolization, remove frame
allowances, or omit failed-visit charging. Each must fail with its specific
semantic diagnostic. Timeout, signal, compilation failure or sanitizer output
never count as a successful control.

The earlier packet rotation/progress and one-attempt shared-retry controls remain.
Existing large-owner and managed-retirement probes follow the extracted helper;
their ownership assertions and mutation controls are preserved.

## Русский

Раньше обязательная очередь получала округлённую вверх треть бюджета. При бюджете
один неподдержанный объект мог забирать каждый проход, не освобождая ничего.
Повторяемый отказ листа также мог вытеснять новые листья и кадры. Теперь четыре
непустые очереди получают отдельные квоты, а остаток распределяется по кругу.
Занятый объект одной очереди больше не расходует квоту другой.

Каждый просмотр головы, включая отказ и незавершённую эпоху читателей, расходует
один шаг. Число просмотров дополнительно ограничено исходным числом готовых
записей. Захват или освобождение целого графа остаётся неделимой операцией с
учётом размера: жёсткой гарантии времени выполнения здесь нет.

Проверка на настоящей VM держит двух соседей активными, настоящие неизвестные
финализаторы — отложенными, а захват каналов — временно отклонённым реальными
дополнительными ссылками и корнями. При этом независимые объекты должны физически
освобождаться. Затем соседи уходят и резервная очистка завершается без потери
владельцев. Отдельные контрольные сборки проверяют каждую устранённую причину
голодания и ограничение числа неудачных просмотров.

Это улучшает продвижение параллельного освобождения. Полный резервный обход всё
ещё требует покоя кучи; полноценная конкурентная трассировка пока не завершена.
Проверки этой серии выполняются на Linux x64; ARM64/RV64 этим не подтверждаются.

## Validation on Linux x64

- 20 selected runtime CTests passed: 13 GC/ownership tests, six further GC and
  module scenarios, and the extended packet/queue fixture. Source-boundary probes
  for large owners, managed retirement and modules were adjusted to the extracted
  helper and revalidated; their semantic controls were retained.
- The queue fixture passed 32 positive VM profiles and seven negative controls.
  The three long native/dedicated/idle GC scenarios also completed successfully.
- TSan passed 188 positive VM profiles: 32 packet/queue cases and 156 large-owner
  cases. Native C/host code and fixture probes are instrumented; emitted JIT
  machine code is not C-TSan instrumented.
- Changed handwritten sources are at most 406 lines; upstream is unchanged.
