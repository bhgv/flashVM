Current architecture and debugger extension: [JIT_ARCHITECTURES.md](JIT_ARCHITECTURES.md)
(v0.348). The x86-64-only limitations below describe the initial v0.346 checkpoint;
five additional Linux ABI encoders are now implemented with separate test evidence.

# Native compiler and execution modes — v0.347 / JIT и режимы исполнения

Runtime selection, switching and exception replay are available through
[execution options](EXECUTION_MODES.md). The low-level explicit API below remains
available for embedding-owned code.

An experimental **Linux x86-64 System V** backend now emits and executes machine
code for an AVM1 subset. Native blocks contain direct calls to compile-time
selected, exception-contained semantic helpers and native conditional/backward
jumps. They do not invoke the AVM1 interpreter or dispatch a decoded instruction
loop at runtime. This is a call-based first tier, without arithmetic inlining or
a performance claim. It is not a complete JIT or completion of roadmap stage 8.

The normal `INTERPRETER` build includes the explicit API in `flashvm/jit.h`.
Build and run the C89 interpreter/native demonstration:

```
cmake -S . -B build
cmake --build build --target flashvm_examples_embedding_jit_main
./build/flashvm_examples_embedding_jit_main
```

It runs `x*2+2` with x=20 through the interpreter, compiles the same body, attaches
it with `fvm_function_set_body`, invokes through the same prepared activation ABI,
checks result 42, unbinds, then frees the code. Unsupported hosts keep interpreting.
No external compiler or assembler is needed at runtime.

## Contract and supported subset

`fvm_jit_compile` copies primitive literals and lowers a body into immutable code.
It changes no function. Attach only after success; on `FVM_UNSUPPORTED`, leave the
original function interpreted. Compile the same bytes used to create the function.
The compiler currently accepts:

- Push: signed integer, float/double, register, terminated string, Boolean, null and undefined.
- StoreRegister and prepared Function2/global/root banks
  (v0.352; [contracts and checks](JIT_REGISTERS.md)).
- ConstantPool and Push-constant8/constant16, with captured/runtime pool selection
  (v0.353; [contracts and checks](JIT_POOLS.md)).
- CallMethod/NewMethod via shared rooted dispatch
  (v0.354; [contracts and checks](JIT_METHODS.md)).
- CallFunction/NewObject with captured lexical and versioned named lookup
  (v0.355; [contracts and checks](JIT_NAMED_CALLS.md)).
- SetTarget/SetTarget2 with versioned target scope and activation restoration
  (v0.356; [contracts and checks](JIT_TARGETS.md)).
- StringLength/Extract, CharToAscii/AsciiToChar and their MB variants; StringAdd,
  StringEquals/Less/Greater (v0.357; [contracts and limits](JIT_STRINGS.md)).
- InitArray/InitObject with shared count normalization and own-entry semantics
  (v0.358; [contracts and checks](JIT_LITERALS.md)).
- Legacy Equals/Less and And/Or with shared versioned coercion/metamethods
  (v0.359; [contracts and checks](JIT_LEGACY.md)).
- End, Return, GetVariable, Pop, PushDuplicate and StackSwap.
- SetVariable, DefineLocal/DefineLocal2, GetMember/SetMember and Delete/Delete2
  (v0.350; [contracts and checks](JIT_REFERENCES.md)).
- Add/Subtract/Multiply/Divide, Modulo, bitwise AND/OR/XOR and the three shifts.
- Add2, Less2, Equals2, StrictEquals and Greater.
- ToInteger, ToNumber, ToString, TypeOf, Increment, Decrement and Not
  (v0.351; [contracts and checks](JIT_NUMBERS.md)).
- Jump and If, with target validation and native forward/backward relocation.

All other operations, including function definitions and exception blocks, return
`FVM_UNSUPPORTED` at compile time. Support in the interpreter does not imply
support in this compiler. Invalid supported record layouts/branch targets return
`FVM_INVALID_ARGUMENT`. Some unreachable unsupported operations can conservatively
reject a body. Source/action/constant/operand/native-code bounds are named in
`src/jit/constants.h`; bounds are implementation limits rather than SWF promises.

Code handles own no managed values and can be reused sequentially across contexts.
They are not thread-safe. Keep a handle alive while attached to any function;
unbind **every** attachment before `fvm_jit_destroy`. Destruction during an active
invocation is rejected. The native-body graph codec already drops process-local
attachments during transfer, preserving original bytecode. No generated code
serialization or hotness policy exists. Runtime JIT mode now compiles/caches
supported bodies automatically (EXECUTION_MODES.md). The `DEFS.txt` JIT build-default
preset and `FVM_FEATURE_JIT` remain unavailable: runtime options work with the
normal interpreter configuration. Other CPU/OS backends return unsupported.

## Lifetime, GC and failures

Compilation validates the body before mapping executable memory. The Linux port
copies into RW pages and seals them RX; it never creates RWX pages or writable
aliases. Native entry includes ENDBR64, saves callee-saved r12 and maintains System
V stack alignment. Every runtime helper that can allocate/throw is called through
an exception-containment wrapper: C++ never unwinds across code without unwind
metadata. Errors return through the common native epilogue.

The prepared function builder retains arguments, locals, this, super and lexical
scopes. A dynamic operand vector is linked as a GC root throughout native execution.
Literal text is copied into context-owned storage before publication. Each source
action charges one shared checkpoint with its original byte offset; loops poll at
the same invocation-local step cadence as the interpreter. Budgets, stop requests,
GC and script exceptions use the existing C services. Failed output writes and
callback side effects follow the ordinary function-invocation contract.

Evidence: `jit_basic` compares interpreter/native host and SWF4–8 results, string
ownership, detachment and bytecode restoration. `jit_control` checks native loops,
forward jumps, empty Return and exact instruction-budget boundaries.
`jit_callbacks` collects from a coercion callback and checks active-code release
rejection, exception status and root/depth restoration. `jit_allocation` walks
compile/invoke allocation failures. `jit_invalid` checks malformed/unsupported
records and unchanged handles. `jit_mapping` checks non-writable executable pages by attempting a write in
an isolated child (proc maps are unavailable in the validation sandbox). The C89 embedding example is also an executable regression.
Memory-instrumented profiles retain their documented restrictions: heap/global
ASan and UBSan only, no stack ASan or LeakSanitizer claim. No other-OS run, other
CPU execution, full Action coverage or Flash Player oracle equivalence is claimed.

## Русский

Добавлен первый работающий backend Linux x86-64: он генерирует машинные блоки,
вызовы общих C-операций и нативные переходы, включая циклы. Интерпретатор из
сгенерированного кода не вызывается. Это явный экспериментальный JIT ограниченного
подмножества, без оптимизаций и без заявления о завершении стадии 8.

Пример `examples/embedding/jit/main.c` можно собрать и запустить командами выше.
Одна функция исполняется интерпретатором и машинным кодом через общий ABI,
получает одинаковые аргументы и возвращает 42. Исходный байткод сохранён.
Неподдерживаемую функцию следует оставить интерпретатору; не надо включать
зарезервированный ключ `JIT` или runtime-флаг `FVM_FEATURE_JIT`.

Код хранится в RW-памяти только при подготовке, затем память становится RX.
Корни стека, лимит инструкций, GC и ошибки согласованы с общими службами VM.
Исключения C++ не пересекают машинный код. Handle должен оставаться живым до
снятия всех привязок; освобождение во время вызова запрещено. В v0.347 добавлены автоматический контекстный кеш и режимы при вызове
(EXECUTION_MODES.md). Нет переноса кода между процессами и backend для остальных ОС/архитектур.
Полный интерпретатор, JIT всех требуемых архитектур и стадии roadmap остаются
открытыми; тесты выше подтверждают только перечисленные сценарии.
