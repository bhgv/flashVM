# Loaded Array.reverse / Reverse в SWF — v0.143

Загруженный reverse использует собственную signed ToInt32 length и raw data
элементов. Прототипы/getter не участвуют. Перед записью обе позиции удаляются;
DONT_DELETE сохраняет descriptor/flags, READ_ONLY запрещает его data-запись.
Getter/setter не вызываются, отсутствующие значения становятся собственными
undefined. Нечётный центральный элемент не меняется; borrowed length сохраняется.
Незагруженный режим сохраняет прежнюю property-aware семантику.

array_legacy_reverse проверяет SWF6..8, borrowed Object, wrap length, наследуемый
индекс, virtual data, DONT_DELETE/READ_ONLY, строки с NUL после GC и реальные Array
holes. Общий лимит 65536 позиций остаётся явным UNSUPPORTED; каждая пара расходует
общий work budget. Нативные C accessor с DONT_DELETE остаются UNSUPPORTED, их
скрытые данные не моделируются. Ошибки после мутации не откатывают её.

## English

Loaded reverse reads own signed ToInt32 length and retained element data, without
prototype lookup or getter invocation. It deletes both positions before storing
own values without setters. DONT_DELETE retains descriptors/flags; READ_ONLY
prevents retained data replacement. Missing values become own undefined slots.
The odd middle position and borrowed length remain unchanged. Unloaded behavior
stays property-aware. Tests cover SWF6..8, borrowed objects, wrapped length,
inherited indices, virtual flags, counted NUL after GC and native Array holes.
The existing 65536-position limit remains explicit UNSUPPORTED, with shared work
budget per pair. Protected native C accessor data is unsupported; failed mutation
is not rolled back. Other mutators and all roadmap stages remain open.
