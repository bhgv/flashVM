# Numeric stepping / Increment и Decrement — v0.287

C89 `fvm_exec_step(context, decrement, input, output)` performs one version-aware
numeric conversion and adds or subtracts the documented unit. It accepts values
and a direction flag, without bytecode, Action IDs, or an interpreter stack.
Zero selects Increment; nonzero selects Decrement. The result is a floating
number, without Int32 wrapping. Fractional values, infinities and NaN retain
ordinary arithmetic behavior. Output may alias input; failures leave it intact.

[GC:] The shared numeric conversion service roots the input through valueOf
callbacks, collection and exceptions. The interpreter keeps operands in its
rooted stack and publishes the result only on success. The new handler does not
add a second conversion, an extra callback, or another root allocation.

The C89 fixture covers SWF4–8, aliasing, fractions, the signed-Int32 boundary,
large finite values, invalid arguments and versioned invalid-string conversion.
SWF4 invalid numeric strings become zero before stepping; SWF5–8 yield NaN.
Callback/action fixtures cover one conversion per operation, otherwise-unrooted
receivers, GC, exception propagation and both Action codes. Allocation injection
checks failure atomicity with an aliased input and root-chain restoration.

Общий C-интерфейс выполняет преобразование один раз, затем прибавляет или
вычитает единицу. Целочисленного переполнения Int32 нет. Вход удерживается при
сборке мусора внутри valueOf; при исключении или отказе памяти выход не меняется.
Интерпретатор вызывает тот же обработчик, который доступен будущему JIT.
Существующая политика числовых преобразований сохранена; эта декомпозиция не
означает появления машинного JIT или завершения стадий роудмапа.
