# Named construction / Имена конструкторов — v0.264

`fvm_exec_new_object` accepts managed scope, target and this inputs, an already
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
`FVM_CALL_SCOPE_SNAPSHOT_COUNT`; проверка глубины `super` использует
`FVM_PROTOTYPE_MAX_DEPTH`. Все девять стадий остаются открытыми.

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
