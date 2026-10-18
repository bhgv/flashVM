# Number radix / Основание Number.toString — v0.191

В загруженных SWF4–8 аргумент radix преобразуется правилами AVM1, включая
explicit undefined. После усечения положительного значения допустимы основания
2–36; остальные значения, NaN и бесконечности выбирают 10 без ошибки.
Переполнение не оборачивается через ToInt32. Строка "010" выбирает 8 в SWF6–8
и 10 в SWF4–5. Ошибки valueOf сохраняются, Object-результат не запускает
дополнительный host toString. Незагруженный C API сохраняет строгую проверку.

number_legacy_radix проверяет эти случаи, дробное основание, порядок callback
и восстановление root/depth state. Эталон: Ruffle number.rs, commit
2372a4e148a67d68efd796f4477b3cddd4f880cd, функция to_string.
В v0.384 добавлена loaded Int32/sentinel-политика значений: [RU](NUMBER_LOADED_RADIX.ru.md). Исторические player-профили NaN/Infinity остаются открытыми.

## English

Loaded SWF4–8 Number.toString coerces every supplied radix through AVM1 rules,
including undefined. Truncated values 2–36 select that base; invalid values,
NaN and infinities fall back to decimal without ToInt32 wrapping. "010" means
8 in SWF6–8 and 10 in SWF4–5. Callback errors propagate and an object result
does not trigger host string fallback. Unloaded C API guards are unchanged.

Tests cover fractional/invalid/overflow radices, version-dependent strings,
callback counts/errors and restored root/depth state. The pinned Ruffle source
above supplies the radix-selection reference. Since v0.384 loaded values use the Int32/sentinel policy: [EN](NUMBER_LOADED_RADIX.md). Historical player-specific nondecimal NaN/Infinity behavior remains open; no stage is closed.
