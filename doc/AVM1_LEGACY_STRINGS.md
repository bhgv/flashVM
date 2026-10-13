# Legacy AVM1 string actions / Старые строковые действия

v0.100 adds StringEquals (0x13), StringLength (0x14), StringExtract (0x15),
ToInteger (0x18), StringAdd (0x21), StringLess (0x29), MBStringLength (0x31),
MBStringExtract (0x35) and StringGreater (0x68). String actions use counted
coercion; equality/order compare UTF-16 units, so equivalent scalar/surrogate
representations compare alike. Addition preserves NUL. Length/extract use UTF-16
units in SWF 6+, including split surrogate pairs. Before SWF 6, ASCII inputs
are supported; non-ASCII length/extract return FVM_UNSUPPORTED until historical
locale-dependent decoding is implemented. This is an explicit remaining gate.

StringEquals/Add/Less convert right first; StringGreater converts left first.
Extract converts length, start, then text. Its start is one-based after Int32
conversion; nonpositive starts begin at zero and negative lengths mean remaining
text. ToInteger uses explicit Int32 wraparound with no undefined floating cast.
Converted strings have a temporary GC root frame across the second callback.

Добавлены девять Action-кодов. Сохраняются NUL и UTF-16 единицы, включая
одиночный суррогат после извлечения. Ошибки и нехватка операндов распространяются
до изменения стека. В тесте второй скриптовый toString вызывает настоящий
gc.collect в traced-string режиме; промежуточная строка первого вызова остаётся
живой. До SWF 6 не-ASCII индексирование требует отдельной реализации кодировок
и не выдаётся за завершённое поведение.

Regression avm1_legacy_string_actions covers counts, NUL, equivalent surrogate
representations, split pairs, one-based extraction/clamping/wraparound, explicit
callback order, integer conversion, unsupported legacy encodings, operand
underflow and repeated collection during script coercion. Other character
conversion/actions, default-hint Add2 and full reference corpus remain open.
Primary cross-check: [pinned Ruffle string actions](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).

## v0.107 — Character conversion actions

CharToAscii (0x32), AsciiToChar (0x33), MBCharToAscii (0x36) and
MBAsciiToChar (0x37) support SWF 6–8 UTF-16 behavior. Character-to-code uses
the first unit (empty/NUL gives zero) and replaces surrogate units with U+FFFD.
Code-to-character wraps numeric coercion to Uint16, returns an empty string for
zero and replaces surrogate units. Before SWF 6, ASCII is supported; non-ASCII
returns FVM_UNSUPPORTED pending byte/locale codec implementation. This limit is
intentional and does not certify the older character encodings.

Четыре операции проверены тестом avm1_character_actions: версии SWF 5–8,
пустая строка, NUL, суррогаты, выход за диапазон, Infinity, ошибки callbacks и
нехватка операндов. Все 268 обычных тестов AGG/Cairo и дополнительный runner
тест AGG прошли, после восстановления прав запуска у пяти бинарников среды.
По три профильных теста прошли в переносимой и ASan/UBSan сборках; ограничения
санитарного профиля сохраняются. Широкий внешний эталон теперь проходит эти
операции, но остаётся незавершённым из-за отсутствующего ActionCall.
