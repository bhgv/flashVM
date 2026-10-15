# Literal construction services / Сервисы построения литералов

v0.343 extracts Array/Object construction from `interpreter.cpp` into
`handlers/literal_array.cpp` and `handlers/literal_object.cpp`. Public C89
functions `fvm_exec_literal_array` and `fvm_exec_literal_object` take a bounded
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
The maximum explicit count is `FVM_EXEC_MAX_LITERAL_ENTRIES` (65536).

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
