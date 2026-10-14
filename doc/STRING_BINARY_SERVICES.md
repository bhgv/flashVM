# Binary string services / Двухместные строковые операции — v0.267

`fvm_exec_string_add` and `fvm_exec_string_compare` provide C89 interfaces over
two values. Each has a separate implementation file. No service receives action
codes, bytecode offsets, interpreter stacks or register banks. String comparison
accepts the existing semantic EQUAL/LESS/GREATER identifiers; other modes fail
validation without changing the output.

StringAdd, StringEquals and StringLess convert the right operand before the left.
StringGreater converts left before right. Comparison uses UTF-16 unit ordering
and returns a Boolean, including loaded SWF4. Concatenation preserves counted
bytes, embedded NUL and lone-surrogate bytes, and publishes a context-owned
result. Inputs and result may alias. Numeric dynamic addition and loose equality
retain their separate services and coercion policies.

[GC:] Both original input graphs and both converted strings are retained in
named root slots. This protects the first converted text when the second callback
collects in traced-string mode. Exceptions, bad modes and allocation failures
preserve the output and restore the caller's root chain.

`execution_string_binary` exercises C89 callers, unloaded/SWF4–8, NUL, raw paired
surrogate concatenation, scalar/pair equivalence and UTF-16 order, GC in both
coercion callbacks, conversion order, exceptions and aliases.
`execution_string_binary_allocation` checks each operation's allocation boundaries.
The existing `avm1_legacy_string_actions` exercises all four interpreter adapters
and repeated collection in the second conversion.

Сложение строк и три строковых сравнения вынесены в общий C-слой. Сохранён порядок
преобразований и точные байты результата, включая встроенный NUL. Имена макросов
описывают исходные/преобразованные корни, число операндов и предел выделения.
GC во втором callback не уничтожает строку первого преобразования. Полная
историческая семантика кодировок и все стадии роудмапа остаются открытыми.

Reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs),
`action_string_add`, `action_string_equals`, `action_string_less` and
`action_string_greater`. Source-based/generated fixtures do not certify complete
Player conformance or JIT equivalence.

A recursion-limit fixture verifies that concatenating loaded legacy undefined
values (whose string spelling is already empty) does not perform a second
ToString on the finished result. Final byte adoption has no conversion callback
or conversion-depth guard; allocation failures release the unadopted buffer.

Validation: GCC13.3 Linux AGG/Cairo each pass 510 tests after successful reruns
for one AGG/five Cairo executables whose execute bits were lost. Six selected
ASan/UBSan fixtures pass with heap/global coverage; stack/leak checks are absent.
All 26 unchanged external SWFs match exact traces in both profiles.
