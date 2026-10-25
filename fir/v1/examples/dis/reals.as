// Numeric AS3 tutorial compiled by our Coco/R frontend and C99 FIR→Dis backend.
// Числовой пример AS3: наш Coco/R frontend и наш C99 FIR→Dis backend.
// This is the documented numeric subset; it does not require an Inferno emitter.
// Это документированное числовое подмножество, без генератора кода Inferno.

// Parentheses compute 5.75 first; multiplication then produces exactly -11.5.
// Скобки сначала дают 5.75; умножение затем даёт ровно -11.5.
trace((2.5 + 3.25) * -2.0);

// Division is binary64, so the fraction is preserved instead of truncated.
// Деление выполняется в binary64: дробная часть не отбрасывается.
trace(7.5 / 2.0); // Captured value / Захваченное значение: 3.75.

// Asymmetric operands make the subtraction order observable.
// Несимметричные операнды позволяют проверить порядок вычитания.
trace(1.0 - 2.25); // Captured value / Захваченное значение: -1.25.
