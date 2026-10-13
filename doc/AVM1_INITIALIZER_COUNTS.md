# Счётчики InitArray/InitObject / Initializer counts — v0.125

В загруженном SWF операнды счётчиков ActionInitArray (0x42) и ActionInitObject
(0x43) преобразуются в Number по правилам AVM1. Отрицательное значение или
значение выше INT32_MAX потребляет только счётчик и кладёт undefined; значения
элементов/пар остаются на стеке. Допустимые дроби усекаются, NaN становится
нулём. Счётчики более 65 536 в допустимом signed-диапазоне пока возвращают
UNSUPPORTED: это явная граница реализации, не ошибка формата. Не загруженный
C-режим сохраняет прежнюю строгую проверку числового целого счётчика.

`avm1_initializer_counts` проверяет оба Action-кода, сохранение стека по Trace,
отрицательную дробь, строковый счётчик, undefined/NaN и границу обхода. Правило
сопоставлено с [pinned initializer implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).

Нехватка операндов всё ещё отклоняется; большие материализованные результаты,
getter/setter прототипов назначения, корни нового Object при преобразовании
имён, версия конструктора/глобального Object и неявный конец блока требуют
следующей работы. Оригинальный init_array_invalid пока не доходит до обработчика:
его DoAction не содержит явного ActionEnd, который текущий инспектор требует.
Его успешное прохождение не заявляется. Все стадии роудмапа остаются открытыми.

## English

Loaded initializer counts use AVM1 numeric conversion. Negative or above-INT32_MAX
counts consume only that operand and push undefined, preserving remaining values.
Valid fractions truncate and NaN becomes zero. Valid counts above 65,536 remain
explicitly unsupported, rather than being labelled malformed. Unloaded raw-C
execution retains its strict integer/count policy.

The named test covers both opcodes, preserved stack traces, negative fractions,
string counts, undefined/NaN and the implementation cap. Rules were checked
against the pinned primary source. Missing operands, large materialization,
destination descriptors, partially built object GC roots/name coercion and
constructor/version provenance remain open. Original init_array_invalid still
fails earlier because its bounded DoAction omits an explicit ActionEnd. No
oracle success or roadmap-stage completion is claimed for that remaining case.

## Последующее исправление / Follow-up v0.126

Неявный конец DoAction исправлен отдельно: оригинальный init_array_invalid
теперь проходит с точным выводом. См. [AVM1_BOUNDED_BLOCKS.md](AVM1_BOUNDED_BLOCKS.md).
The previously documented framing blocker is fixed in v0.126; other initializer
limitations above remain open.
