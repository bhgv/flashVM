# Value-based enumeration / Перечисление свойств — v0.289

C89 `fvm_exec_enumerate(context, input, output)` returns a native Array snapshot
of enumerable names. It accepts values without bytecode, Action IDs or an
interpreter stack. Loaded nonobjects produce an empty Array: Number, Boolean
and String constructors are not called. Objects reuse the bounded native
property enumerator, including its documented name-count bound, current prototype visibility and duplicate
hiding rules. Accessor names are captured without executing their getters.
Counted names with embedded NUL remain intact. Later object mutations do not
change an already returned snapshot. Host-only nonobject input retains the
previous invalid-argument policy.

[GC:] The original input stays rooted until snapshot construction finishes.
The output is published only on success and may alias input; allocation errors
restore the previous root chain. Callers must root or pin the returned Array
across later collections. Loaded snapshots use the cached system Array prototype (or native own methods
when absent) and define own indices without inherited setters. No script
callbacks run during loaded snapshots; host-only construction keeps its existing
prototype lookup policy.

Loaded Enumerate stringifies its variable-name operand once before variable
lookup; the interpreter roots that name during lookup getters. Enumerate2 uses
its value directly. Both append an undefined end sentinel in loaded execution,
including nonobjects and missing stack operands. Unloaded execution retains its
strict operand policy and null sentinel. The interpreter reverses snapshot
iteration when adapting names to the operand stack.

Primary comparison: pinned Ruffle
[Enumerate/Enumerate2 actions](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
and [Value::as_object](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs).
This change preserves the existing native snapshot ordering; complete historical
Flash enumeration order, removed-clip handling and all roadmap gates remain open.

Общий C-обработчик возвращает снимок имён. Примитивы в загруженном SWF дают
пустой список без упаковки и вызова конструкторов. Enumerate преобразует имя
один раз, Enumerate2 использует значение напрямую. В загруженном исполнении
маркер конца — undefined; прежняя строгая политика отдельного host-контекста
сохранена. Проверены SWF4–8, реальные Action-коды, пустой стек, корни при GC,
имена со встроенным NUL, сокрытие свойств, снимки и отказы выделения памяти.
Исключение из преобразования имени распространяется без построения снимка.
Полный исторический порядок перечисления этим изменением не заявляется.
