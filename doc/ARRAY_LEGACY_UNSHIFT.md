# Loaded unshift / Unshift в SWF — v0.152

Загруженный unshift захватывает own signed length, переносит raw данные назад,
удаляет каждую позицию назначения перед записью и не вызывает getter/setter/
watcher. Holes превращаются в собственные undefined даже при нуле аргументов.
Заимствованный Object сохраняет length, но возвращает вычисленную новую длину.
Только нативный Array обновляет длину; READ_ONLY length не запрещает raw слоты
за её пределами. DONT_DELETE/READ_ONLY destination сохраняют descriptor/data.
Строки копируются с counted NUL. Незагруженный режим сохраняет прежний путь.

Общий work scope начинается до приведения length, включая AVM1 valueOf команды.
Это правило применено и к loaded shift/pop. array_legacy_unshift проверяет SWF6..8,
virtual flags без callbacks, prototype holes, borrowed/native readonly length,
GC/countеd строки, ноль аргументов, budget 64 и combined valueOf+движение для
unshift/shift. Scope/корни восстанавливаются после бюджетной ошибки.

Отрицательная signed length, старые длины более 65536 и новая signed длина
более INT32_MAX возвращают UNSUPPORTED до перестановки. Native C accessor raw
данные, case/version rules и все стадии открыты. Ошибка после начала мутации
может оставить изменения; откат не обещается. Public ABI/wire не меняются.

## English

Loaded unshift captures own signed length, moves raw values backwards, deletes
destinations before storage and bypasses getters/setters/watchers. Holes become
own undefined even with zero arguments. Borrowed length stays unchanged while
the computed new length is returned. Only native Arrays resize; readonly length
allows raw slots beyond it. Protected destinations retain descriptors/data.
Counted strings are owned. Unloaded behavior remains unchanged.
Work scope starts before length coercion, charging AVM1 valueOf and traversal
together; loaded shift/pop now do this too. SWF6..8 tests cover virtual flags,
prototype holes, borrowed/native readonly length, counted strings/GC, zero args,
64-step budget and combined valueOf+unshift/shift work, with root/scope unwind.
Negative signed lengths, old length above 65536 and new signed length above
INT32_MAX remain UNSUPPORTED. Native C accessor data, case/version and every
stage stay open. Failed mutation may leave prior changes; no rollback is
promised. Public ABI and wire format remain unchanged.
