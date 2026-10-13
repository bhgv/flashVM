# Границы splice / Loaded splice bounds — v0.133

В загруженном SWF start и deleteCount преобразуются общим AVM1 Number-помощником
и оборачиваются в signed Int32. Поэтому start 2^32 означает 0, start 2^32−1
означает −1, deleteCount 2^32+1 означает 1, а deleteCount 2^32−1 отвергается
как отрицательный с результатом undefined без изменения массива. Каждое
преобразование выполняется ровно один раз в прежнем порядке start → length →
deleteCount. Незагруженный C-режим сохраняет обычное усечение.

array_legacy_splice_bounds проверяет SWF4..8, число и строку, отрицательный
wrapped start, отрицательный wrapped deleteCount и однократный valueOf.
Используется общий помощник signed индексов, уже проверенный для slice.

Это отдельное исправление границ, а не полная историческая семантика splice:
чтение длины ещё property-aware/unsigned, removed holes и accessor/move/set_data
требуют следующей работы. Большие mutating обходы ограничены. Остальные правила
конструкторов/дескрипторов/версий и все стадии роудмапа остаются открытыми.

## English

Loaded splice start/deleteCount use AVM1 Number conversion and signed wrapping
Int32: 2^32 start wraps to zero, 2^32−1 start to minus one, 2^32+1 deleteCount
to one, and 2^32−1 deleteCount is rejected as negative without mutation.
Conversions run once each in start → length → deleteCount order. Unloaded host
mode keeps ordinary truncation. array_legacy_splice_bounds covers SWF4..8 numeric
and string operands, negative wrapping and a single valueOf call.

This fixes endpoints only. Length is still property-aware/unsigned; removed
holes and historical accessor/move/set_data behavior remain open, as do large
mutating traversals, constructor/descriptor/version rules and every stage.
