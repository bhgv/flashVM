# Date construction and UTC / Конструктор Date и UTC — v0.199

Конструктор Date и Date.UTC используют общую с instance methods обработку
loaded аргументов: не более семи, слева направо, до первого undefined. Числовая
проекция AVM1 исключает повторные вызовы valueOf. Date.UTC после преобразования
возвращает undefined, если осталось меньше двух аргументов. Конструктор с
пустым обработанным списком выбирает текущее время. Ошибки передаются до
создания экземпляра; host поведение, callback IDs и формат графа сохранены.

## English

Loaded Date construction and Date.UTC share the instance-method operand prefix:
at most seven AVM1 numeric conversions, left to right, stopping before undefined.
Numeric projection prevents duplicate callbacks. UTC returns undefined with
fewer than two projected arguments; an empty constructor prefix selects current
time. Conversion errors propagate before instance creation. Unloaded behavior,
native callback IDs and graph/API versions remain unchanged.

Tests cover SWF4–8, seven-value cutoff/callback counts, numeric reference inputs,
missing/undefined UTC operands, ignored errors after undefined, octal timestamp
strings, current-time selection, conversion errors and GC/channel restored class
invocation/construction. Shared prototype allocation-failure tests also run.
Reference: pinned Ruffle date.rs method/constructor/utc, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd. Date() global call semantics, precise
current-time rounding, calendar arithmetic and all roadmap stages remain open.
