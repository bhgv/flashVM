# String indices / Индексы String — v0.182

Загруженные charAt/charCodeAt, substring/slice/substr и fromCharCode используют
AVM1 numeric coercion и signed ToInt32: усечение к нулю, modulo 2^32, NaN/Infinity
в ноль. Object.valueOf не получает host toString fallback. Substr вычисляет
signed wrapping конец и relative clamp, включая историческое отрицательное
количество. Portion без аргументов возвращает undefined до receiver coercion.
charCodeAt SWF5 возвращает 0 для неотрицательного out-of-range индекса; отрицательный
индекс по-прежнему NaN. Незагруженные правила сохранены.

string_legacy_indices проверяет SWF4..8/host: дробное/отрицательное переполнение,
2^31, Infinity, octal строку, Object fallback, SWF5 out-of-range, отрицательный
substr count, substring overflow, отсутствие аргументов, fromCharCode и глубины.
Индексы поиска, split limits, все аргументы/boxed overrides и все стадии
роудмапа остаются открытыми.

## English

Loaded character/portion/fromCharCode methods use AVM1 numeric coercion and
signed ToInt32: truncate toward zero, wrap modulo 2^32, map NaN/infinities to zero.
Object.valueOf does not get host toString fallback. Substr wraps the signed end
and applies relative clamping, including historical negative-count behavior.
Argument-free portions return undefined before receiver coercion. SWF5 charCodeAt
returns zero for nonnegative out-of-range indices; negative indices remain NaN.
Unloaded behavior is preserved.

Tests cover SWF4..8/host fractional/negative overflow, 2^31, infinity, octal syntax,
Object fallback, SWF5 boundaries, negative substr count, substring overflow,
missing arguments, fromCharCode and restored depths. Search indices, split limits,
full argument/boxed override semantics and all stages remain open. Reference:
pinned AVM1 globals/string.rs coerce_to_i32/string_wrapping_index/char_code_at.
