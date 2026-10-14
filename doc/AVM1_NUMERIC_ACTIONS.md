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

## v0.88 — SWF 5/6 string truth

Not, And, Or и If используют числовую истинность строк для загруженных SWF
5/6, а для SWF 7/8 — непустую строку. Без загруженного SWF сохраняется
современный режим. Ошибки числового преобразования не меняют стек.
Тест avm1_version_truth загружает минимальные SWF каждой версии 5–8 и
проверяет четыре Action-кода, нули, дроби, NaN, Infinity и встроенный NUL.
SWF 4 и старше, исторический разбор чисел и версии отдельных функций остаются открытыми.

Loaded SWF 5/6 use numeric string truth in Not/And/Or/If; SWF 7/8 and
contexts without loaded metadata use nonempty string truth. Conversion errors
propagate before stack mutation. The new test loads minimal SWFs 5–8 rather
than editing private context metadata. Pre-SWF-5 truth, exact historical
number parsing and per-function SWF version provenance remain open.
Reference cross-check: [pinned Ruffle value implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs).

## v0.93 — Loaded-version numeric grammar

Действия AVM1 используют отдельный числовой мост для загруженного SWF.
До SWF 7 null/undefined дают 0, далее NaN. Строковый hex появляется в SWF 6;
hex/octal оборачиваются в Int32. Сохраняется исторический сдвиг raw hex-prefix
(0x-10 допустим, +0x10 даёт NaN), октальная форма без пробелов, строгий хвост
с SWF 5, prefix-разбор и 0 при ошибке до SWF 5. Infinity как строка не принимается.
Экспонента оборачивается в Int32; отсутствующие цифры экспоненты означают 0.
Числовая истинность строк ниже SWF 7 использует этот же путь. До SWF 5
строковое boolean даёт 1/0; Trace сохраняет особое undefined.

AVM1 numeric conversion now uses loaded-version primitive gates and counted
legacy syntax separately from the embedding C API. Raw hexadecimal/octal use
explicit Uint32 arithmetic and signed Int32 results; decimal exponent parsing
also wraps without signed overflow. Embedded NUL is never a terminator.
The host decimal mantissa converter supplies the floating result; exact historical
rounding, whitespace tables and object fallback policy are not fully certified.
Objects still use FlashVM's existing numeric primitive fallback, then apply
version-aware conversion to the returned primitive. Per-function player version
provenance remains open. Contexts without SWF metadata retain host semantics.

avm1_version_numbers tests version gates, radix/sign/wraparound, decimal syntax,
NUL, object-returned strings and unchanged C embedding conversion. Three original
external SWFs now complete with byte-for-byte pinned expected output:
undefined_to_string_swf6 (6), swf6_string_as_bool (6), equals_swf5 (5).
This is targeted reference evidence, not full AVM1 compatibility.

## v0.95 — SWF 8 unsigned-shift compatibility

В загруженном SWF 8 URShift возвращает Int32, включая эффективный сдвиг 0.
В SWF 5–7 и контексте без SWF сохраняется Uint32. Все битовые операции
и Modulo преобразуют правый операнд первым; ошибки останавливают следующий
вызов. Это уточняет ранее описанный порядок и безусловное Uint32 правило.

Loaded SWF 8 URShift uses a signed result, while SWF 5–7/unloaded contexts
retain unsigned results. The effective count remains masked to five bits.
Bitwise/remainder coercion now runs right first, keeping both operands rooted.
avm1_version_shifts covers signed boundaries, count 0/32, conversion order and
right-side failure. Original SWF 8 biturshift_swf8 completes and exactly matches
its pinned 93-byte reference output. Earlier notes about always-unsigned URShift
and left-first remainder are superseded by this verified behavior.

## v0.96 — Legacy arithmetic coercion

Старые Add/Subtract/Multiply/Divide преобразуют операнды через AVM1 number
bridge вместо отказа для нечисловых типов. Add/Multiply преобразуют левый
операнд первым, Subtract/Divide — правый. Стек удерживает оба корня до
успешного завершения hook/преобразований. В SWF 4 деление на ноль даёт
строку #ERROR#; более поздние версии сохраняют IEEE результат.

Legacy arithmetic now accepts coercible operands with per-opcode callback
order and preserves FlashVM metahooks. Conversion failures occur before operand
consumption. SWF 4 division by either sign of zero returns #ERROR#. Regression
avm1_legacy_arithmetic covers SWF 4–8, numeric strings/booleans, prefix syntax,
zero signs, valueOf order, native GC barriers and early failure. The original
equals_swf4 reference still fails on an uncoercible global object; full object
conversion policy and remaining actions are open.

## v0.97 — Numeric valueOf without string fallback

Загруженный AVM1 вызывает только valueOf для числового преобразования
объекта. Отсутствующий/невызываемый valueOf даёт undefined; возвращённый
объект не преобразуется повторно (0 до SWF 5, NaN далее). MovieClip не
вызывает valueOf в этом пути. Счётчик primitive-depth ограничивает повторный
вход и восстанавливается после ошибок. Это заменяет ранее отмеченный
незавершённый object fallback; C API и unloaded режим сохраняют свои правила.

Loaded AVM1 numeric object conversion performs valueOf-only lookup/invocation
with a bounded reentry scope. It does not call toString after an object result
or recursively coerce that result. Missing/noncallable valueOf is undefined.
avm1_object_number tests omitted/primitive/object-returned methods, legacy
primitive gates, numeric returned strings, GC barriers and recovery after
recursive conversion failure. Original equals_swf4 now finishes its whole action
block but still differs because unversioned installed NaN/Infinity globals exist
in SWF 4; the full reference is not yet passing. Per-version standard global
installation and complete display-object coercion remain open.

## v0.103 — Unordered relational results

Less2/Greater return undefined for numeric NaN rather than false. Finite results
remain booleans; a non-display object result still takes the early false path.
Greater converts the right operand first, while Less2 converts left first.
This supersedes the earlier left-first Greater notes. Metahook boolean behavior
remains the FlashVM extension contract.

Эталонные большие lessthan2 SWF 5–7 выявили отличие типа результата. Новый
avm1_relational_results проверяет undefined для NaN/нечисловых строк, boolean
для определённых сравнений и порядок valueOf. GC регрессия проверяет обе
стороны Greater после изменения порядка. Полные внешние сравнения пока
расходятся из-за this/root и других версионных правил, поэтому не закрыты.
Primary cross-check: pinned Ruffle abstract_lt/action_greater implementations
linked in this document; original external expected traces also require undefined.

Subsequent correction / Позднейшее исправление v0.229: loaded StrictEquals also
recognizes a pair of numeric NaNs. See AVM1_NUMBER_TEXT.md and the unchanged
strictequals_swf6 oracle; earlier StrictEquals distinctions describe the old implementation.
