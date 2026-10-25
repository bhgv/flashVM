# Named construction / Имена конструкторов — v0.264

[`fvm_exec_new_object`](../include/flashvm/execution.h#L598) accepts managed scope, target and this inputs, an already
coerced counted name and ordered argument values. It does not inspect bytecode,
operand stacks or interpreter-local maps. The interpreter still converts the
name before the argument count and preserves its unloaded count-validation rule.

Loaded NewObject uses ordinary literal-name resolution, including the versioned
implicit this rule. It does not apply GetVariable/CallFunction path parsing:
`space.Ctor` selects a literal scoped name and `space:Ctor` is also literal.
Missing, scalar and nonconstructable loaded values yield undefined; deleted
bindings are not revived through script/native constructor registries. Installed
standard constructors remain available through their actual global bindings.
Unloaded host execution retains qualified lookup and named native-constructor
fallback. This corrects the former interpreter's qualified loaded construction.

[GC:] Environment, constructor and argument copies remain rooted through lookup
and invocation. A new allocation-free object root retains a script constructor's
fresh instance while its `prototype` getter runs. A getter-triggered collection
previously freed that instance before its prototype assignment; the new fixture
reproduced the heap-use-after-free under ASan before this fix.

`execution_new_object` covers C89 callers, unloaded/SWF4–8 names, scripted native
constructor bodies, prototype getter/body GC, thrown constructors, missing/scalar
values and output preservation. `avm1_new_object_names` covers interpreter literal
versus qualified names and deleted definitions. `execution_new_object_allocation`
injects allocation failures and verifies root/invocation restoration. Full boxed
primitive construction, encoding and scope semantics remain subject to the wider
compatibility inventory.

В загруженных SWF `NewObject` ищет буквальное имя в области видимости; разбор
путей `GetVariable` здесь не применяется. Общая C-служба принимает только объекты
окружения и значения. Временно защищён новый экземпляр между его выделением и
входом в тело конструктора, включая сборку мусора внутри getter `prototype`.
Имена и аргументы удерживаются при callback, исключении и отказе памяти.
Число корней снимка вызова вынесено в документированный макрос
[`FVM_CALL_SCOPE_SNAPSHOT_COUNT`](../src/core/call_scope_constants.h#L7); проверка глубины `super` использует
[`FVM_PROTOTYPE_MAX_DEPTH`](../src/core/numeric_constants.h#L51). Все девять стадий остаются открытыми.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`action_new_object`/`resolve`, and
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
`construct`. Generated fixtures are source-based probes, not independent Player
conformance certification.

Validation: Linux GCC13.3 AGG and Cairo each pass 505 tests. Eight AGG binaries
required execute-bit restoration and a successful rerun. All 26 unchanged
external SWFs have exact matching stdout and empty stderr in both profiles.
Seven selected ASan/UBSan fixtures pass with heap/global instrumentation;
stack and leak checks are unavailable in this environment. No other-platform
or generated-machine-code execution is claimed.

## v0.345: constructor count preparation / Подготовка числа аргументов

[`fvm_exec_constructor_count`](../include/flashvm/execution.h#L648) now supplies NewObject count preparation as an
explicit C89 service. It accepts a count value, number of available arguments
and output count, without reading bytecode or a stack. Name conversion remains
before count conversion in the interpreter. In an unloaded host, the count must
be an integral Number, fit available arguments and not exceed
[`FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS`](../include/flashvm/execution.h#L17) (65536). Invalid host counts return
[`FVM_ERROR`](../include/flashvm/flashvm.h#L45). Loaded SWFs delegate to [`fvm_exec_argument_count`](../include/flashvm/execution.h#L640): AVM1 conversion,
UInt32 wrapping and capping to available values precede the 65535 method bound.
This preserves the deliberately distinct host and loaded policies.

The loaded conversion roots its input through script callbacks; all failures
preserve the output count. Host numeric validation allocates nothing. The obsolete
literal-bound macro was removed from the bytecode encoding header: constructor
limits now use the semantic public .h constant rather than a literal-layout name.

В v0.345 подготовка числа аргументов NewObject вынесена в C-сервис без стека
и пи-кода. Сохранены строгий host-предел 65536 и loaded-предел 65535 после
преобразования UInt32 и ограничения доступными аргументами. Имя по-прежнему
преобразуется первым. При ошибке выходной счётчик не меняется; вход удерживается
через callbacks/GC. Host-проверка не выделяет память. Это декомпозиция существующей
семантики, а не завершение интерпретатора или стадии JIT.

Evidence: `execution_constructor_count` covers C89 host/SWF4–8, the two boundary
policies, availability, signed/fractional/nonfinite inputs, wrapping, foreign values
and invalid pointers. It also feeds 65536 prepared arguments into direct Array
construction in a host context. `execution_constructor_count_callbacks` collects
from script valueOf and checks exceptions/root restoration.
`execution_constructor_count_allocation` walks loaded allocation failures and
checks host validation with allocation disabled. Existing raw named-count and
NewObject-name tests cover interpreter integration and coercion order.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_CALL_SCOPE_SNAPSHOT_COUNT` | macro / define | [src/core/call_scope_constants.h:7](../src/core/call_scope_constants.h#L7) |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_exec_argument_count` | function | [include/flashvm/execution.h:640](../include/flashvm/execution.h#L640) |
| `fvm_exec_constructor_count` | function | [include/flashvm/execution.h:648](../include/flashvm/execution.h#L648) |
| `FVM_EXEC_MAX_CONSTRUCTOR_ARGUMENTS` | macro / define | [include/flashvm/execution.h:17](../include/flashvm/execution.h#L17) |
| `fvm_exec_new_object` | function | [include/flashvm/execution.h:598](../include/flashvm/execution.h#L598) |
| `FVM_PROTOTYPE_MAX_DEPTH` | macro / define | [src/core/numeric_constants.h:51](../src/core/numeric_constants.h#L51) |
<!-- source-links:end -->
