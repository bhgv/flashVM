# Fast splice budget / Бюджет быстрого splice — v0.493

Быстрый native splice в загруженных SWF4–8 теперь учитывает каждый
просмотренный stored index исходного массива и каждый вставляемый аргумент.
Ранее учитывался только проход по удалённому диапазону. Плотное копирование
без удаления и чистая вставка могли обойти instruction_limit и проверки
остановки worker. Учитываются также записи, пропущенные из-за выхода за
ранее вычисленную длину после callback преобразования аргумента.

The fast staging path shares the existing execution-work scope with its caller.
Its charged units are visited source indexed entries + deleted result positions
+ inserted arguments. It does not walk logical holes just to copy a sparse
source: a million-element logical array with two stored entries and no removal
needs two units for source staging. Rejected overloads retain their previous
behavior; unloaded native staging remains unmetered by this loaded-SWF scope.

Instruction exhaustion or worker stop during staging returns [FVM_ERROR](../include/flashvm/flashvm.h#L43) before
publishing the staged index table. The temporary removed array is rooted through
staging and released on failure. Native call/frame/execution depth is restored;
GC reclaims unpublished objects and retry with sufficient budget succeeds.
Argument-coercion callbacks run before staging and may already have changed the
source; those changes are retained on a later staging error. These statements
apply to the staged fast path, not arbitrary generic mutations or all VM phases.

`array_splice_fast_budget` exercises loaded SWF4–8 through C calls, direct
function invocation, interpreter, explicit and automatic native JIT: dense
copying, insertion-only and removed-hole staging; unchanged staged source,
shared instruction limits, cleanup, collection and retry. It also checks the
unloaded embedding policy. The direct regression fails against v0.492.

`array_splice_fast_sparse` checks exact physical-entry work on a large sparse
source and an ignored tail added by delete-count coercion. Callback changes
survive the budget failure. `array_splice_fast_budget_allocation` injects each
new/new[] boundary until execution reaches the budget failure, checks unchanged
staged storage and counted NUL strings under traced string collection, collects
and retries.
`array_splice_fast_stop` runs actual OS workers for both previously unmetered
phases in every SWF4–8 version; self-requested stop is observed through the
normal shared polling mechanism even with instruction_limit disabled.

Проверки относятся к текущему Linux-хосту и его нативному JIT. Это развитие
учёта работы/кооперативной остановки FlashVM, а не измерение времени Flash Player.
Полная приёмка worker/GC/debugger, остальные операции, другие ОС/архитектуры
и все девять стадий роудмапа остаются открытыми. ABI/wire не меняются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
<!-- source-links:end -->
