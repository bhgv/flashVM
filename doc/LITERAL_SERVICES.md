# Literal construction services / Сервисы построения литералов

v0.343 extracts Array/Object construction from `interpreter.cpp` into
`handlers/literal_array.cpp` and `handlers/literal_object.cpp`. Public C89
functions [`fvm_exec_literal_array`](../include/flashvm/execution.h#L41) and [`fvm_exec_literal_object`](../include/flashvm/execution.h#L48) take a bounded
entry count and ordinary values, never Action codes, bytecode or an operand
stack. They can be called by C extensions and future generated bodies.
The adapter still decodes/coerces the count, pops entries in AVM1 order,
supplies undefined for loaded stack underflow and commits the result to its stack.

Array entries are supplied in logical index order. Every element becomes an own
slot, even undefined; a numeric entry is never interpreted as Array length.
Object entries are supplied in assignment order. Object construction precedes
name conversion; each name conversion precedes its setter. Loaded SWFs use AVM1
string conversion and ignore empty converted names. Hosts retain empty names.
Counted keys, duplicate writes and existing property-set semantics are retained.
The maximum explicit count is [`FVM_EXEC_MAX_LITERAL_ENTRIES`](../include/flashvm/execution.h#L20) (65536).

Both services validate all input ownership, copy values before callbacks and
root the complete snapshot and partial result. They permit collecting script
callbacks, while preserving any existing enclosing native barrier. Output may
alias an input and changes only on success. Allocation and callback errors
restore roots and preserve output; earlier property writes and arbitrary script
side effects are not rolled back. Input value copies retain managed references;
borrowed string bytes must remain valid for the call as with other value APIs.

В v0.343 создание литералов Array/Object вынесено в общий C-интерфейс без
байткода и стека интерпретатора. Адаптер подготавливает значения и обрабатывает
нехватку операндов; сервисы создают массив со своими индексами или объект
с последовательным преобразованием имён и вызовом setter. Имена с NUL,
повторные присваивания и версия-зависимая обработка пустого имени сохранены.
Все входные значения копируются и удерживаются корнями до callbacks. При
ошибке результат вызывающего кода не меняется; побочные эффекты скрипта
и ранее выполненные setter не откатываются. Это не завершает этапы roadmap.

Evidence: `execution_literals.c` is a C89 caller covering host/SWF4–8, own
undefined slots, numeric elements, ordered duplicate keys, counted NUL keys,
empty names, output aliases, foreign values and invalid/bounded inputs.
`execution_literals_callbacks.cpp` invokes GC from a bound script body while
mutating the original entry buffer; both future entries and the partial result
remain live. It also checks exceptions, unchanged output and restored root/depth
state. `execution_literals_allocation.cpp` walks allocation failures through both
services with aliased outputs. Existing initializer underflow/empty-name and
Array inherited-property suites remain regression coverage. These are local
regressions, not a new Flash Player oracle comparison or JIT backend validation.

## v0.344: literal count conversion / Преобразование счётчика

[`fvm_exec_literal_count`](../include/flashvm/execution.h#L36) now owns count coercion as a separate value-only C
operation. It returns a normalized integer Number, or undefined when loaded
AVM1 rejects a negative/out-of-signed32 count. This sentinel tells the adapter
to consume only the count, leaving entry operands untouched. Loaded NaN becomes
zero; nonnegative fractions are floored. Counts above 65536 but within the
signed range return [`FVM_UNSUPPORTED`](../include/flashvm/flashvm.h#L46). Unloaded hosts retain their previous
strict policy: non-Number input is [`FVM_ERROR`](../include/flashvm/flashvm.h#L43), invalid numeric input is
[`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Neither mode constructs an object or consumes operands.
Errors preserve output; aliases are allowed. Coercible inputs are retained
through script and GC. No interpreter activation is required.

В v0.344 преобразование счётчика также вынесено из цикла интерпретатора.
C-сервис возвращает целое число либо undefined для отклонённого счётчика
загруженного SWF. В последнем случае адаптер сохраняет операнды элементов.
NaN становится нулём, неотрицательная дробь округляется вниз, превышение
предела материализации даёт UNSUPPORTED. Строгие host-проверки и их коды
ошибок сохранены. Результат при ошибке не меняется, вход удерживается корнем
через callbacks и GC. Лимиты и расположение корней именованы в .h.

Evidence: `execution_literal_count.c` covers host/SWF4–8, aliases, fractional,
NaN, infinite and signed/resource-bound cases plus invalid inputs.
`execution_literal_count_callbacks.cpp` collects from a script `valueOf`,
checking live receiver, exceptions and root/depth restoration.
`execution_literal_count_allocation.cpp` walks count-conversion allocation
failures with an aliased input. Existing raw Action count/underflow tests
verify that rejected counts leave entry operands untouched. This is regression
coverage of existing behavior; roadmap stages and JIT backend validation remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `fvm_exec_literal_array` | function | [include/flashvm/execution.h:41](../include/flashvm/execution.h#L41) |
| `fvm_exec_literal_count` | function | [include/flashvm/execution.h:36](../include/flashvm/execution.h#L36) |
| `fvm_exec_literal_object` | function | [include/flashvm/execution.h:48](../include/flashvm/execution.h#L48) |
| `FVM_EXEC_MAX_LITERAL_ENTRIES` | macro / define | [include/flashvm/execution.h:20](../include/flashvm/execution.h#L20) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
