# Sort arguments / Аргументы сортировки — v0.173

Для загруженных Array.sort числовые flags усечены, преобразованы по модулю
2^32 и ограничены пятью известными битами. NaN/Infinity дают нулевые flags.
В перегрузке с двумя Number второй выбирает flags. После comparator Object
нечисловой второй аргумент игнорируется без преобразования. Неподходящий
первый примитив возвращает undefined без сортировки. Array.sortOn без
аргументов возвращает undefined. Незагруженные правила сохранены.

array_legacy_sort_arguments проверяет SWF4..8, дробные/высокие/циклические
flags, NaN/Infinity, строку/boolean/null/undefined, два Number и пустой sortOn.
Полные перегрузки sortOn, невызываемый comparator Object, корни во время
произвольного native GC, Unicode case mapping и все стадии остаются открытыми.

## English

Loaded Array.sort truncates numeric flags, wraps modulo 2^32 and keeps five
known bits. NaN/infinities become zero flags. Two Number arguments select the
second number. A nonnumeric second argument after a comparator Object is
ignored without coercion. An unsuitable primitive first argument returns
undefined without sorting. Argument-free Array.sortOn returns undefined.
Unloaded rules are preserved.

array_legacy_sort_arguments covers SWF4..8, fractional/high/wrapping flags,
NaN/infinities, string/boolean/null/undefined, two numbers and empty sortOn.
Full sortOn overloads, noncallable comparator Objects, arbitrary native callback
GC, full Unicode case mapping and all stages remain open. Reference: pinned
Ruffle globals/array.rs sort and sort_on argument dispatch.
