# Scoped deletion / Удаление переменной

v0.250 adds the previously unsupported Delete2 action for unqualified names.
The action converts its name using loaded-version string rules, then deletes the
first visible binding in retained activation/With scopes, selected target and
global fallback. It leaves a Boolean result. Qualified dot/colon paths explicitly
remain unsupported in this checkpoint; this does not claim full Delete2 support.

The C89 service [`fvm_exec_scope_delete`](../include/flashvm/execution.h#L363) accepts managed scope/target objects and a
coercible name, without bytecode, stack or local-map arguments. Presence is checked
without reading values or invoking getters. An inherited With/target member or
nondeletable binding blocks deletion of an outer same-name variable. Activations
use their own binding cells; their prototype edge leads to the enclosing scope.

[GC:] Documented uppercase root slots retain scope, target, source name and
converted text through callbacks. Outputs may alias names; failures leave output
and the candidate binding unchanged. Tests cover SWF4–8 globals/activation locals,
With inheritance, protected properties, counted NUL names, getter suppression,
collection/exception in name conversion and allocation-failure root restoration.

Primary comparison: Ruffle commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
core/src/avm1/activation.rs action_delete_2 and core/src/avm1/scope.rs Scope::delete.
No independent Flash Player run is implied by these implementation comparisons.
General case/version/path acceptance and all roadmap stages remain open.

Добавлена Delete2 для простых имён. Общая C-служба находит первую привязку,
не вызывая getter, и соблюдает запрет удаления. Унаследованное свойство With
блокирует удаление одноимённой внешней переменной. Корни удерживают области
видимости и имя во время преобразования, включая GC/исключение. Составные
пути пока явно возвращают UNSUPPORTED; полная совместимость не заявляется.

Evidence: 458 Linux AGG/Cairo tests pass per profile; one Cairo execute-bit launch
failure passes after permission recovery. Seven focused heap/global ASan+UBSan
checks pass (no stack/leak instrumentation). All 23 unchanged SWF<=8 oracle traces
remain exact per profile. The retrieved upstream delete2 probe is SWF16, so it is
preserved unchanged and excluded from SWF<=8 acceptance, not rewritten or counted.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_scope_delete` | function | [include/flashvm/execution.h:363](../include/flashvm/execution.h#L363) |
<!-- source-links:end -->
