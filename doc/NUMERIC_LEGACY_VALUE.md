# Numeric value methods / Методы значений Number и Boolean — v0.190

Загруженные Number/Boolean valueOf/toString на receiver другого native типа
возвращают undefined без преобразования receiver. Number.toString в этом
случае не преобразует radix. Типизированные boxed значения возвращают stored
data независимо от overridden conversion methods. Неверный owner/повреждённый
native state и незагруженные guards сохранены.

numeric_legacy_value проверяет SWF4..8/host, Object/String/другой boxed primitive/
Array/Function, аварийные receiver conversions и radix.valueOf, оба метода,
stored Number/Boolean при override, traced GC и восстановление root/depth state.
Prototype layouts, полная radix semantics, отдельные версии функций/клипов и
все стадии остаются открытыми.

## English

Loaded Number/Boolean valueOf/toString on the wrong native receiver type return
undefined without coercion; Number.toString does not evaluate radix in that case.
Typed boxed receivers return stored data independent of instance conversion
overrides. Invalid owners/corrupt native state and unloaded guards are preserved.

Tests cover SWF4..8/host Object/String/other primitive/Array/Function receivers,
trap conversions/radix.valueOf, both methods, overridden stored Number/Boolean,
traced GC and restored root/depth state. Prototype layouts, full radix semantics,
separate function/clip versions and all stages remain open. Reference: pinned
number.rs/boolean.rs native-type checks before value/format operations.
