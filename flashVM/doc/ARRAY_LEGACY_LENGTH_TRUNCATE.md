# Array.length truncation / Сокращение длины — v0.172

В загруженных SWF4..8 native Array присваивание неотрицательной целой длины
сначала удаляет индексы от новой до прежней длины, затем записывает length.
DONT_DELETE сохраняет свойство за пределами длины без увеличения length;
READ_ONLY length запрещает запись длины, но не предшествующее удаление хвоста.
Индексы вне прежней длины не удаляются. Общий бюджет ограничивает обход;
ошибка сохраняет уже выполненные удаления. Незагруженный режим сохранён.

array_legacy_length_truncate проверяет оба режима, protected tails, readonly
length, частичное удаление при исчерпании бюджета и восстановление корней.
Строка с NUL в хвосте переживает traced GC и передачу через канал даже после
снятия DONT_DELETE и повторного сокращения длины до нуля.
Полное преобразование ToInt32, отрицательная/циклическая длина, виртуальная
native length и все стадии роудмапа остаются открытыми.
Границы удаления уже используют signed индексы: длины от 2^31 до 2^32−1
дают отрицательное значение, новая граница ограничивается нулём, старая
не создаёт диапазон удаления. Хранимая длина пока сохраняет прежний контракт.

## English

Loaded SWF4..8 native Array assignment of a nonnegative integer length deletes
indices from the new length to the old length before storing length. DONT_DELETE
retains an outside property without clamping length. READ_ONLY length suppresses
length storage while allowing prior tail deletion. Slots already outside the old
length are untouched. The shared work budget bounds traversal; failure retains
completed deletions. Unloaded behavior is preserved.

array_legacy_length_truncate covers both modes, protected tails, readonly length,
partial budget failure and root restoration. A counted NUL string survives traced
GC/channel transfer after clearing DONT_DELETE and shrinking again to zero.
Full ToInt32 conversion, negative/wrapping lengths, virtual native length and all
roadmap stages remain open. Reference: pinned Ruffle ScriptObject.set_local.
Deletion bounds already use signed indices: lengths from 2^31 to 2^32−1 wrap
negative, clamping the new bound to zero and yielding no old deletion range.
Stored length retains the existing contract.
