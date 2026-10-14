# SetVariable name conversion / Преобразование имени — v0.277

Loaded SWF4–8 SetVariable accepts values for its name operand and converts them
once with the version-aware AVM1 string policy. Already counted strings avoid
an additional conversion and retain embedded NUL. Host-only C calls retain the
existing requirement for a String name. Empty names remain ignored.

The common `fvm_exec_variable_set` service owns conversion, path parsing and
binding selection; interpreter dispatch only adapts stack operands. No bytecode,
program counter or operand stack enters the C interface. The additional named
root slot retains converted text separately from its original input. Scope,
target, receiver and assigned value are rooted before a conversion callback can
collect garbage, throw or change bindings. Conversion exceptions propagate before
assignment. Failed allocation restores root frames and conversion depth.

В загруженных SWF имя преобразуется один раз внутри общего C-обработчика.
Значение для присваивания и объекты области видимости удерживаются до вызова
пользовательского toString. Исключение из преобразования не выполняет запись.
Интерфейс подходит для прямого вызова из будущего сгенерированного кода; это
не означает наличия машинного JIT или завершения стадий роудмапа.

`execution_set_variable_names` exercises C89 host/loaded SWF4–8, numeric names,
counted NUL, object conversion with GC, assigned-object survival and exceptions.
`execution_set_variable_names_allocation` injects allocation failures through
conversion and storage. `avm1_set_variable_names` covers real action dispatch
with numeric names in SWF4–8. Existing scope/path and With fixtures also apply.

Primary reference: Ruffle pinned at
[`action_set_variable`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs),
which pops the assigned value/name and coerces the name before assignment.
Full mutable-this, removed-clip and target mutation semantics remain open.

Validation: GCC13.3 Linux AGG/Cairo each pass 529 tests. Execute-bit restoration
and successful reruns were required for 7/5 binaries respectively (BAD_COMMAND).
Seven affected heap/global ASan+UBSan checks pass, without stack/leak coverage.
All 26 unchanged original SWF traces match exactly in both profiles.
