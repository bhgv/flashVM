# Qualified Delete2 / Составные имена при удалении

v0.251 adds dot/colon member paths to Delete2. The interpreter resolves the first
binding, then [`fvm_exec_delete_path`](../include/flashvm/execution.h#L371) traverses the remaining counted path from
that base value. Intermediate object getters may run; the final member is deleted
without being read. Missing/primitive prefixes report false. DONT_DELETE and
inherited-final-member behavior remain those of the shared delete-member service.

The C89 service accepts base/path values, without scopes, bytecode or a VM stack.
A slash in a final member name is literal; slash syntax in object prefixes still
requires a separate display-target resolver and returns UNSUPPORTED. Full target
path grammar, outer-scope retry after a partial path miss, case/version corner
cases and pathless slash references remain open. A documented segment bound is
checked before traversal getters execute. These cases do not certify complete
Flash variable-path behavior.

[GC:] Retain both the original base and the changing current object, path text,
next getter result and final name. Getters may detach earlier graph links and
collect. Counted text survives callbacks; output aliases are allowed and errors
preserve the output. Allocation injection checks root restoration and no final
property mutation on failure.

[GC:] The known addProperty getter bridge now omits only its own native collector
barrier after rooting receiver, lookup owner and descriptor snapshot. Prepared
script/native-script getters can collect. Ordinary C getters still block GC, as
do enclosing host native callbacks; their barriers are never suspended. Getter
self-deletion and prototype/descriptor detachment retain exact entry identities.
The setter bridge keeps its previous barrier in this checkpoint.

Tests cover SWF4–8 mixed separators, primitives/missing prefixes, C89 traversal,
embedded NUL/final slash names, final getter suppression, script getter GC and
exceptions, self-detachment, original-base retention, outer C barriers, native
getters, bounds and allocation failure. The upstream delete2 SWF16 remains
excluded from the SWF<=8 acceptance corpus.

Delete2 теперь поддерживает цепочки свойств с точкой и двоеточием. Общая C-служба
удерживает исходный объект и промежуточные значения, не читает удаляемое свойство
и безопасно проходит скриптовые getters. Известный мост addProperty допускает GC
после установки корней; внешний нативный барьер и обычные C-getters сохраняют
запрет. Проверяются самоудаление дескрипторов, исключения и отказы памяти.
Пути MovieClip со слешами и полная грамматика/поиск путей остаются открытыми.

Evidence: 461 tests pass per Linux AGG/Cairo profile after five AGG execute-bit
launch failures are recovered. Sixteen focused heap/global ASan+UBSan checks pass;
stack/leak instrumentation remains unavailable. All 23 prior original probes stay
exact. The new SWF7 delete probe matches using its declared three-frame policy.
The expanded 25-probe set is 24/25: SWF8 prototype_delete still formats MovieClip's
prototype as a display path; this trace mismatch remains recorded as open work.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_delete_path` | function | [include/flashvm/execution.h:371](../include/flashvm/execution.h#L371) |
<!-- source-links:end -->
