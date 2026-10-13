# Loaded shift/pop / Удаление элементов в SWF — v0.144

Загруженные shift/pop читают own signed length и raw data, без getter/prototype.
Shift удаляет каждую позицию назначения перед прямой записью, включая undefined
вместо holes. Заимствованный shift сохраняет последний индекс и length; pop
удаляет последний собственный индекс, сохраняя borrowed length. Только нативный
Array уменьшает длину. DONT_DELETE не удерживает native length: защищённый индекс
может остаться за её пределами. READ_ONLY length сохраняет прежние данные длины.
Незагруженный режим не меняется.

array_legacy_remove проверяет SWF6..8: borrowed Object, inherited hole, защищённый
virtual descriptor, отсутствие getter/setter вызовов, counted NUL и GC, нативные
Array с DONT_DELETE последним индексом. Отрицательная signed length явно
UNSUPPORTED; shift более 65536 позиций также UNSUPPORTED и ограничен work budget.
Native C accessor retained data, watcher ordering и остальные mutators открыты.
Ошибки после начала мутации не откатывают предыдущие изменения.

## English

Loaded shift/pop read own signed length and raw element data without getters or
prototype lookup. Shift deletes each destination before direct storage, including
own undefined holes. Borrowed shift retains the last index and length; borrowed
pop deletes its last own index without resizing. Only native Arrays shrink.
DONT_DELETE does not clamp native length: a protected index may remain beyond it.
Readonly native length retains its data. Unloaded behavior stays unchanged.
Tests cover SWF6..8, borrowed objects, inherited holes, protected virtual data,
no accessor calls, counted NUL/GC and protected native Array tail indices.
Negative signed lengths and shift above 65536 positions remain UNSUPPORTED;
shift shares the work budget. Native accessor data, watcher order, other mutators
and every stage remain open. Partial mutation is not rolled back on error.
