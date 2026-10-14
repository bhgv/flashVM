# Plain comparator / Невызываемый comparator — v0.176

Загруженный Array.sort принимает обычный Object в позиции comparator.
Его вызов даёт undefined, числовое сравнение — равенство. Свойства call и
valueOf самого comparator не вызываются. DESCENDING по-прежнему разворачивает
полный результат, RETURNINDEXEDARRAY возвращает исходные позиции без записи,
UNIQUESORT использует встроенное сравнение соседних значений, а не comparator.
Вызываемые Function/C callable продолжают выполняться как раньше; незагруженный
невызываемый Object по-прежнему даёт UNSUPPORTED.

array_legacy_sort_plain_comparator проверяет SWF4..8 и незагруженный режим,
обычный Object с аварийными call/valueOf, flags 0/2/8/10/4/12, порядок данных
и индексов, отсутствие indexed copyback, уникальные/повторяющиеся значения и
восстановление root/depth state. Полные Array/AVM1 семантика, произвольный native
callback GC и все стадии роудмапа остаются открытыми.

## English

Loaded Array.sort accepts a plain Object comparator. Calling it yields undefined,
so numeric comparison treats operands as equal. Its call/valueOf properties are
not invoked. DESCENDING reverses the whole result; RETURNINDEXEDARRAY returns
original positions without writing the receiver; UNIQUESORT uses builtin
adjacent-value comparison rather than the comparator. Callable Function/C objects
retain existing dispatch. Unloaded noncallable Objects still return UNSUPPORTED.

Tests cover SWF4..8/unloaded mode, trap call/valueOf methods, flags 0/2/8/10/4/12,
data/index ordering, indexed no-copyback, distinct/duplicate values and restored
root/depth state. Full Array/AVM1 semantics, arbitrary native callback GC and all
stages remain open. Reference: pinned ScriptObject.call and globals/array.rs
sort_compare_custom/sort_internal.
