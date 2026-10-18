# String search / Поиск в строках — v0.183

Загруженные indexOf/lastIndexOf преобразуют needle через AVM1 string coercion,
позицию через signed ToInt32. Нет аргументов — undefined, но после receiver
coercion. Явный undefined позиции сохраняет default 0/length. Отрицательная
позиция обратного поиска возвращает -1; forward позиция за длиной возвращает
-1 даже для пустой needle. NaN/Infinity позиции преобразуются в ноль, а не
default конца строки. Незагруженная семантика сохранена.

string_legacy_search проверяет SWF4..8/host, missing/undefined needle, версионную
undefined строку, Object needle, порядок receiver/missing arguments и ошибку,
NaN/Infinity/overflow/negative позиции, explicit undefined и empty needle.
Глубины и GC roots восстановлены. Split limits, boxed overrides, независимая
полная сверка Flash Player и все стадии остаются открытыми.

## English

Loaded searches use AVM1 needle coercion and signed Int32 positions. Missing
arguments return undefined after receiver coercion. An explicit undefined position
retains the default start/end. Negative reverse positions return -1; forward
positions beyond length return -1 even for an empty needle. NaN/infinities wrap
to zero instead of defaulting to the end. Unloaded semantics are preserved.

Tests cover SWF4..8/host, missing/undefined and Object needles, versioned undefined
strings, receiver-before-missing ordering/errors, NaN/infinity/overflow/negative
positions, explicit undefined and empty needles; root/depth state is restored.
Split limits, boxed overrides, independent full Flash Player verification and all
stages remain open. Reference: pinned globals/string.rs index_of/last_index_of.

Since v0.382 loaded scan positions/comparisons share native execution work: [EN](STRING_LOADED_WORK.md) / [RU](STRING_LOADED_WORK.ru.md).
