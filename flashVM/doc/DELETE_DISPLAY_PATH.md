# Explicit-base display deletion / Удаление по пути от заданного объекта

v0.255 extends the existing C89 [`fvm_exec_delete_path`](../include/flashvm/execution.h#L495) service. If a slash occurs
before the final dot/colon separator, the preceding counted prefix uses the
MovieClip path resolver. Resolution starts at the explicit base, follows children
before dynamic properties and honors its existing absolute/parent/version rules.
After resolution the final counted property is deleted without reading its getter.
A slash in the final property remains literal. Missing/nonobject prefixes report
false; getter exceptions and allocation failures preserve the caller's output.

[GC:] The complete input, original base, resolved object and final member remain
rooted across callbacks. Resolver root positions, traversal limits and the legacy
ASCII bound now use documented uppercase constants in `movieclip_path_constants.h`.
The resolver still rejects unverified non-ASCII legacy folding. Its traversal and
prototype bounds remain unchanged.

This is an explicit-base service for C/native generated bodies. The interpreter's
Delete2 namespace selection still rejects slash prefixes before its first lexical
separator: outer-scope retries and complete GetVariable grammar remain open. The
service does not mutate selected targets or accept bytecode/interpreter stacks.

Общий C-обработчик разрешает префикс со слешами от переданного объекта, затем
удаляет последнее свойство без вызова его getter. Сохранены правила приоритета
дочерних клипов, родительских путей и версий SWF. Строки поддерживают явную длину.
Результат не меняется при исключении или нехватке памяти. Полная интеграция
путей Delete2 с поиском внешних областей остаётся открытой, как и стадии 1–9.

Validation: 468/468 Linux AGG and 468/468 Cairo checks, with four/three launch
failures recovered by restoring executable permissions. Four selected heap/global
ASan + UBSan checks passed (host stack/leak coverage disabled). All 25 unchanged
reference traces matched in both profiles. Tests cover SWF4–8 child paths, parent
paths, dotted clip names, counted NUL/literal slash members, getter GC/exception,
outer native barriers and allocation failures before deletion/root restoration.
Проверено по 468 тестов AGG/Cairo, четыре выбранных проверки памяти и 25 эталонов
в каждом профиле; ограничения инструментов и оставшаяся семантика указаны выше.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_delete_path` | function | [include/flashvm/execution.h:495](../include/flashvm/execution.h#L495) |
<!-- source-links:end -->
