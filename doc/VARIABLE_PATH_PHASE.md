# Shared variable-path phase / Общий разбор путей переменных

v0.258 exposes [`fvm_exec_variable_path`](../include/flashvm/execution.h#L398) as a C89 value service and uses it in the
interpreter's counted variable-read bridge and Delete2 owner lookup. This removes
their duplicate path grammar. Managed scopes/target/this and a counted string are
explicit inputs; bytecode, operand stacks and raw local maps are not accepted.

Qualified references split at the last colon (SWF4) or dot/colon (SWF5+ and host
mode). Empty object prefixes return a handled undefined result. Otherwise scoped
member lookup distinguishes a missing final property from a present undefined.
SWF5+ bare slash paths return a resolved object; if none resolves, handled=0 asks
the caller to try the whole name literally. Simple names also return handled=0.
In that case out stays unchanged, including when it aliases the name, and there
is no root-array allocation. Every error preserves out/handled/present.

The interpreter keeps its established plain-name/implicit-this lookup after an
unhandled path. Counted constructor/type variable references use the same bridge.
CallFunction's separate named receiver lookup and SetVariable assignments retain
their previous implementations; full path/case/call equivalence remains open.
The resolver recognizes the _global namespace when retrying from the actual
global object; nearer explicit bindings can still resolve first.

[GC:] Qualified traversal retains original scope, target, explicit this, full name,
prefix/member slices and the unpublished result. Getter GC/exception, descriptor
detachment and aliased output are exercised at this public service boundary.
Allocation failures preserve result flags and restore the caller's root chain.
Reference syntax/bounds now use documented FVM_REFERENCE_PATH_* macros in a shared
header, independently from operation-specific root layouts.

Общий C-этап подключён к GetVariable и разрешению владельца Delete2. При обычном
имени или неразрешённом пути со слешем он просит продолжить буквальный поиск,
сохраняя выходное значение. Квалифицированный путь различает отсутствие свойства
и существующий undefined. Сохранены правила SWF4/5 и корни во время getter/GC.
Нативное тело может вызвать ту же C-службу. SetVariable, отдельный поиск получателя
CallFunction и полная историческая совместимость остаются открытыми.

Evidence: 480/480 Linux AGG and 480/480 Cairo tests; six AGG and two selected
sanitizer execute-bit launch failures passed after permission restoration.
Fourteen selected heap/global ASan + UBSan checks passed; stack/leak coverage
remains disabled on this host. All 25 unchanged reference SWFs matched in each
profile. This verifies the listed path subset, not full AVM1/Flash equivalence.
Проверено по 480 тестов AGG/Cairo, 14 выбранных проверок памяти и 25 исходных
эталонов в каждом профиле. Ограничения совместимости и инструментов сохранены.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_variable_path` | function | [include/flashvm/execution.h:398](../include/flashvm/execution.h#L398) |
<!-- source-links:end -->
