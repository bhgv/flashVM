# Numeric Array.length / Числовая длина Array — v0.492

В загруженных SWF4–8 присваивание Number в native `Array.length` сохраняет
исходное число: отрицательные значения, дроби, signed zero, NaN, бесконечности
и числа за границами Uint32. Для удаления элементов и обхода отдельно
вычисляется wrapping Int32; исходное значение не заменяется результатом
преобразования. Существующие host-only проверки конструктора/length и прежний
путь преобразования нечисловых аргументов не расширены этой версией.

One allocation-free helper interprets numeric length by truncation, modulo 2^32
and signed decoding. NaN/infinities map to zero for traversal. Loaded array
size queries, raw data length and raw indexed growth use that interpretation;
no unchecked floating-to-integer cast is used for extreme numeric storage.
Property-aware paths handle noncanonical raw numbers and Number metatables.
Consequently high unsigned raw lengths wrapping negative sort as empty ranges
rather than hitting the previous native-sort rejection.

Readonly length still permits tail deletion before protected length storage.
DONT_DELETE entries can remain beyond the interpreted length. Successful writable
assignment clears version bits and retains the Number's primitive metatable.
Read-only methods and sort/reverse preserve raw length; methods actually writing
length create a canonical integer Number and replace its metatable. Zero-length
pop/shift do not write length. Positive indexed growth also replaces the old
numeric metatable. GC traces numeric metatable cycles through native storage.

Graph encoding/decoding accepts any Number payload for native Array and validates
its primitive metatable through the normal graph identity map. Other native
payloads retain their previous no-metatable rule. Invalid non-Number Array state
and invalid graph references remain rejected. Wire layout/public ABI are
unchanged; pre-v0.492 readers reject newly permitted fractional/nonfinite/out-of-
range native lengths and numeric payload metatables.

Tests: `array_numeric_length` covers SWF4–8, independent literal Int32 bounds,
C/AVM1/interpreter/explicit/automatic native JIT writes, readonly/nondelete/version
flags, signed zero, primitive metatables, GC and host graph restoration.
`array_numeric_methods` covers ten methods, preservation versus normalization,
readonly length, fractional/extreme numbers and metadata. `array_numeric_wire`
checks bitwise IEEE transport, a one-record metatable cycle and CRC-valid invalid
references before destination object allocation. `array_numeric_length_allocation`
injects every new/new[] boundary into truncation and decoder restoration,
collects after errors and retries partial mutations. The direct length regression
fails against v0.491. The old malformed-state check now rejects non-Number tags;
the large-sort fixture checks valid empty-range behavior on negative Int32 length.

Проверена числовая ветка. Произвольные нечисловые и виртуальные length,
полный разбор знаковых index-имён, переполнение мутаций, реальный Flash Player
и запуски на остальных целевых ОС ещё не закрыты. Ошибка удаления/памяти/
бюджета может оставить ранее выполненные удаления. Все девять стадий открыты.

Primary comparison sources: pinned Ruffle
[Array set_local/length/set_length](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
and [Array methods](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
This is source-based comparison, not a fresh original-player oracle run.

Current extension / Обновление v0.527: the original native accessor also retains
nonnumeric values with separate AVM1 coercion; graph payloads now accept every
valid value type. See [ARRAY_TYPED_LENGTH.md](ARRAY_TYPED_LENGTH.md) for the new
contract, tests, older-reader compatibility and remaining descriptor limitations.
