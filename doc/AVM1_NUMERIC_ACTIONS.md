# Числовые Action-коды / Numeric actions

Добавлены Increment (0x50), Decrement (0x51), Modulo (0x3f), BitAnd/Or/Xor
(0x60–0x62), LShift/RShift/URShift (0x63–0x65), Greater (0x67).
Преобразование операндов выполняется слева направо через существующий ToNumber
или ToPrimitive. Ошибка преобразования возвращается до изменения стека.
Битовые аргументы приводятся к Uint32, сдвиг ограничивается младшими пятью
битами. Результаты знаковые Int32, кроме URShift (Uint32). Арифметический
сдвиг реализован масками без зависимости от signed-shift поведения компилятора
и ширины long. Modulo сохраняет знак остатка/нуля; деление на ноль, NaN и
бесконечный делимый дают NaN. Greater сохраняет порядок преобразования,
сравнивает строки по UTF-16, поддерживает __gt как расширение FlashVM.

avm1_numeric_operations проверяет крайние биты, знаки, wraparound, сдвиги 0/31/
32/33/-1, строки, дроби, NaN/Infinity/-0, порядок valueOf, запрет повторного GC
в нативном вызове, распространение ошибки и восстановление последующих вызовов.
Проверяются также недостающие операнды. Сопутствующие AVM1 тесты остаются в наборе.
Это не завершение AVM1: другие Action-коды, правила версий, области видимости и
полная эталонная сверка остаются открытыми. Modulo/bitwise hooks расширений пока
не добавлены; эти операции используют стандартный путь преобразования.

Increment/decrement, remainder, six bit operations and Greater are implemented.
Coercion observes left-to-right order. Unsigned arithmetic and explicit masks
avoid signed-shift and host long-width assumptions; URShift returns Uint32 while
other bit operations return signed Int32. Remainder handles nonfinite values,
zero divisors and signed zero. Greater preserves coercion order and supports the
FlashVM __gt extension. The new suite covers boundary values, coercion errors,
native GC reentry rejection and subsequent execution. Other opcodes, version
semantics, scope conformance and full Flash acceptance remain open.

## v0.87 — Legacy comparison and logic

Старые Equals (0x0e) и Less (0x0f) преобразуют операнды через текущий
ToNumber вместо отказа для строк и объектов. Equals вызывает преобразование
правого операнда первым, Less — левого. Операнды остаются в корнях стека
до завершения метаметода/преобразований; ошибки распространяются. And/Or
принимают остальные типы через текущую проверку истинности, без valueOf.
Историческая истинность строк до SWF 7 и версия-зависимый ToNumber пока открыты.

Legacy Equals/Less use the existing numeric conversion path with right-first
Equals and left-first Less coercion. Both operands remain stack roots until
completion. And/Or accept other value types without object coercion. Existing
FlashVM comparison metahooks remain supported. Regression tests cover numeric
strings, NaN, booleans, object conversion order and non-coercing logical actions.
Pre-SWF-7 string truth and full version-specific numeric coercion remain open.
Order cross-check: [Ruffle AVM1 activation at pinned commit](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
This is an implementation comparison, not a captured Flash Player oracle.
