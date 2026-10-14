# Common primitive conversions / Общие преобразования значений

v0.254 exposes existing AVM1 Number/legacy Integer, Boolean and TypeOf semantics
as C89 value services in separate handler files. The interpreter retains only
operand movement and operation selection. Generated bodies can use the same
interfaces without instruction IDs, bytecode or an interpreter stack.

`fvm_exec_to_number` uses loaded-version conversion and optionally signed Int32
truncation/wrapping. NaN/infinity map to zero in integer mode; integer zero has no
negative sign, as in the previous bit-conversion implementation. Ordinary Number
conversion preserves its current version-specific NaN/null/string policies.

`fvm_exec_to_boolean` preserves pre-SWF7 numeric-string truth and newer nonempty
string truth. It executes no user callbacks and introduces no root allocation.
`fvm_exec_type_of` returns immutable borrowed labels without callbacks/allocation.
It preserves current callable/resource classification, including the still-open
builtin-prototype/type distinctions; this is not a full typeof compatibility fix.

[GC:] Number conversion roots its input through valueOf/native script callbacks.
All outputs may alias inputs and remain unchanged on failure. Boolean/type
classification does not enter the collector. Tests cover C89 versions/aliases,
signed wrapping and zero, NaN/infinity, native script GC/exception and allocation
failure/root restoration. Existing operator/branch/original-SWF checks remain.

Общие C-службы выполняют ToNumber/ToInteger, преобразование истинности и TypeOf.
Числовое преобразование удерживает аргумент во время callback, сохраняет результат
при ошибке и корректно нормализует знак нуля в целочисленном режиме. Boolean и
TypeOf не вызывают пользовательский код. Классификация типов сохранена прежней;
её оставшиеся исторические различия не объявляются исправленными. JIT и общая
приёмка всех стадий остаются открытыми.

Validation: 467/467 Linux AGG and 467/467 Cairo checks; the AGG run required
restoring executable bits and rerunning three launch failures. Ten selected
heap/global ASan + UBSan checks passed (stack/leak coverage disabled in this
host). All 25 unchanged reference SWFs matched their oracles in both profiles.
Проверка: по 467 тестов AGG/Cairo, 10 выбранных проверок памяти, 25 исходных
эталонных SWF в каждом профиле; полная совместимость этим не подтверждается.
