# Number.toString: основания 2/4/8/16/32 / Power-of-two radices

Добавлена точная запись конечных double для оснований 2, 4, 8, 16 и 32,
включая дроби, числа выше 2^53, максимальное double и субнормальные значения.
Целая часть раскладывается делением на основание; дробная — умножением,
пока остаток не исчезнет. Для этих оснований операции точны в двоичном double.
Буферы ограничены максимальным числом цифр IEEE binary64, без STL и зависимости
от ширины long. -0 отображается как 0; NaN/Infinity используют прежний путь.

standard_number проверяет 0.25 во всех пяти основаниях, -10.625, DBL_MAX,
минимальную субнормальную величину, сохранение десятичного пути и перенос
дробного Number через канал. Поддержка остальных оснований пока ограничена
целыми числами до 2^53-1: точное округление периодических дробей ещё открыто.
Полная сверка форматирования с версиями Flash и завершение этапа 1 не заявлены.

Finite binary64 values now have an exact representation in bases 2/4/8/16/32,
including fractions, large integers and subnormals. Bounded stack buffers and
floating-point digit extraction avoid host integer-width assumptions and STL.
Existing decimal and nonfinite formatting remains in use. The standard_number
suite covers fractions, signs, extreme values and transported native methods.
Recurring fractions in other bases and historical Flash formatting conformance
remain open; roadmap stage 1 is not complete.
