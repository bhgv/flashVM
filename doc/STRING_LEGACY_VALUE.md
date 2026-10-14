# String value methods / Методы получения значения — v0.186

Загруженные String.prototype.valueOf/toString, вызванные на не-String Object,
возвращают undefined. Они не преобразуют receiver и не вызывают его toString/
valueOf. Boxed String возвращает сохранённую строку, включая NUL, независимо
от overridden методов самого instance. Неверный owner/повреждённое native String
state по-прежнему INVALID_ARGUMENT; незагруженный non-String guard сохранён.

string_legacy_value проверяет SWF4..8/host, Object/Number/Boolean/Array/Function
receivers с аварийными conversion methods, оба borrowed value methods, boxed
NUL value при override, traced GC и восстановление состояния. Полная семантика
boxed coercion overrides, prototype data/case и все стадии остаются открытыми.

## English

Loaded String valueOf/toString calls on non-String Objects return undefined
without receiver conversion or invocation of its conversion methods. Boxed
Strings return stored counted data regardless of instance overrides. Invalid
owners/corrupt native String state still return INVALID_ARGUMENT; the unloaded
non-String guard is preserved.

Tests cover SWF4..8/host Object/Number/Boolean/Array/Function receivers with trap
conversion methods, both borrowed methods, overridden boxed NUL values, traced
GC and restored state. Full boxed coercion overrides, prototype data/case and all
stages remain open. Reference: pinned to_string_value_of native String matching.
