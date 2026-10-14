# String.split / Разбиение строк — v0.184

Loaded limit использует AVM1 ToInt32, отрицательное значение ограничено нулём.
Delimiter проходит AVM1 string coercion после limit, даже при нулевом limit.
SWF5: отсутствующий/undefined delimiter означает comma, explicit empty delimiter
возвращает всю строку. Для отсутствующего effective delimiter whole-string branch
не применяет limit, как в эталоне. Прочие версии сохраняют отсутствующий delimiter
как whole string и empty delimiter как UTF16 units. Host правила сохранены.

string_legacy_split проверяет SWF4..8/host, default/empty/undefined delimiter,
negative/2^31/wrapping/infinity limits, boolean версии 4, Object delimiter,
порядок limit→delimiter, ошибки обеих фаз и восстановление root/depth state.
Cap 65536 частей сохранён; полнота boxed overrides, исключительных ресурсов,
независимая сверка Flash Player и все стадии остаются открытыми.

## English

Loaded limits use AVM1 signed Int32, clamping negatives to zero. Delimiters use
AVM1 string coercion after limit conversion, even with zero limit. SWF5 defaults
missing/undefined delimiters to comma and treats explicit empty delimiters as a
whole-string request. The absent-effective-delimiter branch ignores limit, as
the reference does. Other versions retain whole-string/UTF16-unit behavior for
absent/empty delimiters. Host rules are unchanged.

Tests cover SWF4..8/host default/empty/undefined delimiters, negative/2^31/wrapping/
infinite limits, SWF4 boolean conversion, Object delimiters, limit-before-delimiter
ordering, errors in either phase and restored root/depth state. The 65536-part
cap remains; full boxed overrides, exceptional resources, independent original
Flash Player validation and all stages remain open. Reference: pinned split.
