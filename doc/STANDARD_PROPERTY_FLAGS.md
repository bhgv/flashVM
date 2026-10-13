# ASSetPropFlags precedence / Приоритет флагов — v0.146

ASSetPropFlags сначала очищает clear, затем устанавливает set. Пересекающийся
бит остаётся установленным: результат `(old & ~clear) | set`, как в pinned
ScriptObject::set_attributes. Исправлен только стандартный AS builtin; прямое
расширение C для установки флагов не меняет существующий контракт.

standard_property_flags — C89 тест всех 512 сочетаний old/set/clear в каждом
из незагруженного режима и SWF4..8. Также проверены counted NUL, readonly data,
перечень отсутствующих/унаследованных свойств и NULL-перечень всех собственных
свойств. Приведение аргументов, недостающие аргументы, маски версий, различие
NULL/undefined и case handling всё ещё требуют отдельной совместимости.
Все стадии остаются открытыми.

## English

ASSetPropFlags clears before setting: overlapping bits remain set, producing
`(old & ~clear) | set` as in pinned ScriptObject::set_attributes. Only the AS
builtin changes; the direct C extension preserves its existing contract.
The C89 test exhaustively checks 512 old/set/clear combinations in unloaded mode
and each SWF4..8, plus counted NUL, readonly data, missing/inherited named fields
and NULL selection of all own fields. Argument coercion/defaults, version masks,
NULL/undefined distinction, case handling and every roadmap stage remain open.
