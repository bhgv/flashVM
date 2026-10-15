# Missing constant-pool entries / Отсутствующие константы — v0.311

Loaded ActionPush now produces undefined when a Constant8 or Constant16 index is
outside the current string pool, including an absent/empty pool. Existing valid
indices still create owned strings. Standalone host execution retains its strict
FVM_INVALID_ARGUMENT result. Truncated index payloads remain malformed in every
mode: the decoder checks the full index width before examining the pool.

The widths have separate documented FVM_AVM1_CONSTANT8_INDEX_BYTES and
FVM_AVM1_CONSTANT16_INDEX_BYTES macros in encoding_constants.h. No shared C
execution service or ABI changes are needed for this bytecode decoding rule.

avm1_constant_missing checks host/SWF4–8, both widths, absent/one-entry pools,
index zero, the first miss, maximum encoded indices, valid string results,
continuation after a miss and truncated payloads. Function return checks expose
the actual pushed value rather than relying only on successful execution.
avm1_constant_missing_allocation injects preparation failures for both missing
index widths, with GC between attempts and restored root/execution chains.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs action_push ConstantPool branch. An invalid index
produces Value::Undefined there. Full string decoding, constant-pool lifetime and
all historical player behavior are not certified by these bounded fixtures.

Загруженный Push при выходе Constant8/Constant16 за границу таблицы теперь кладёт
undefined и продолжает исполнение. Корректные индексы сохраняют прежнее поведение;
усечённые поля остаются ошибкой формата. Проверены SWF4–8 и строгий host-режим,
пустая и заполненная таблицы, граничные индексы, возвращаемое значение, продолжение
и отказы памяти. Размеры полей вынесены в отдельные смысловые макросы.
Все стадии роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 601 tests; five AGG launch permissions
were restored and those tests passed on retry. Two targeted heap/global
ASan+UBSan tests passed (no stack/leak coverage). All 26 accepted original SWFs
matched the pinned oracle in both profiles. Action-reference coverage is 94/100.
