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

## v0.147 — selector/defaults / перечень и значения по умолчанию

Только NULL выбирает все свойства. Остальные значения приводятся к строке: в
загруженном SWF используется версионный AVM1 bridge. undefined выбирает пустое
имя в SWF4..6 и "undefined" в SWF7..8; незагруженный режим выбирает "undefined".
Без объекта/перечня функция возвращает undefined без изменения флагов;
отсутствующие set/clear равны нулю. Существующий C89 тест дополнен каждой из
этих веток, включая host empty property, созданное до загрузки SWF.
Полное преобразование receiver и числовых масок, большие маски/версии и case
handling остаются открытыми; текущая числовая валидация разрешает лишь integer 0..7.

Only NULL selects all own fields. Other selectors undergo string conversion,
using the versioned AVM1 bridge when loaded. Undefined selects empty in SWF4..6
and "undefined" in SWF7..8/unloaded mode. Missing object/list returns undefined
without flag changes; absent set/clear masks default to zero. The existing C89
test covers each branch, including a host empty field created before loading.
Full receiver/mask coercion, wider/version masks and case handling remain open;
mask validation still accepts only integers 0..7. Every stage remains open.
