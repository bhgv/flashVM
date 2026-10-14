# Value-based method calls / Вызовы методов через значения

v0.240 extracts the shared CallMethod/NewMethod operation into
`src/avm1/handlers/call_method.cpp`. The interpreter retains operand decoding
and stack movement. C89 callers use `fvm_exec_call_method` with owner/name values,
explicit anonymous receiver, ordered arguments and a construction flag. No
instruction address, opcode or interpreter stack crosses this interface.

The service preserves existing dispatch: primitive boxing, named native/object
methods, super dispatch, anonymous callable values, constructor members and the
legacy named-script fallback. Null/undefined/empty names select an anonymous
call; other non-string names are rejected. This preserves current behavior,
not a claim of complete historical name-coercion semantics. Context runtime
services still supply prototypes, scope, version policy and exception state.

[GC:] Root slots retain owner, counted name, explicit receiver, primitive box,
constructor and arguments across callbacks. Prepared native script bodies can
collect; ordinary native callbacks retain their existing collection barrier.
Errors leave the result unchanged. Root slots and the argument bound have
commented uppercase macros in dedicated/public headers.

C tests cover embedded-NUL names, explicit this, aliased output, primitive boxing,
null receivers, validation, named construction and collection from a prepared
native body. Allocation injection checks boxing failures, unchanged output and
root-frame restoration. This is an interface for future generated code; there
is still no machine-code JIT backend.

Крупный обработчик вызова/создания вынесен из интерпретатора. Общая C-служба
принимает значения и аргументы в порядке вызова; декодирование байткода и
изменение стека остаются в основном цикле. Корни GC защищают промежуточные
значения, включая упакованный примитив и конструктор, полученный из свойства.
Проверяются прямые вызовы из C89, сборка из подготовленного нативного тела,
создание объекта и отказы памяти. Правила преобразования нестандартных имён
и прочие пробелы совместимости остаются открытыми; JIT ещё не реализован.

Validation: 445/445 tests in both AGG and Cairo profiles; 23/23 unchanged SWF
oracles in each profile. Five selected heap/global ASan+UBSan checks passed.
The local sanitizer environment does not provide stack/leak instrumentation.
Windows, macOS and Android were not executed here.
