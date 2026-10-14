# Shared local declarations / Общий обработчик локальных объявлений

v0.262 adds C89 `fvm_exec_variable_declare` with explicit managed scope, activation,
target, this, initialization mode, coercible name and optional value. DefineLocal,
DefineLocal2 and named function publication use the same handler. The interpreter
retains operand movement, target selection and mapping its current frame root to
the managed activation; raw local maps are not part of the public service.

Loaded initialized declarations update an own property of the nearest With object,
otherwise continue to the first enclosing local activation or target. An inherited
With property alone does not select that receiver. Uninitialized declarations first
check the current object's applicable prototype presence without reading getters;
if absent, declaration follows the same own-With/enclosing-local search. This can
write undefined to an existing outer binding when an inner With object lacks it,
following the reference implementation's current-object check.

At top level, dot/colon declarations use shared path assignment. The uninitialized
variant first performs the corresponding variable read and skips an existing
binding, including a present undefined. This qualified read can invoke getters or
throw; it is different from the getter-free simple declaration check. In local
activations, separator-containing declaration names are literal. Loaded empty names
are ignored after coercion. Unloaded host activation declarations retain their
prior own-data/literal-name policy, now copying borrowed string initializers.

Fresh local cells do not dispatch descriptors on enclosing lexical activations:
the managed prototype link represents the enclosing scope. Existing own local
setters still use the property bridge. Raw local __proto__ data does not replace
the lexical parent link. Complete user-prototype/local-inheritance/casing behavior
remains a separate scope-model boundary, not certified by these fixtures.

[GC:] Original scope, activation, target, this, coercible name, initializer,
converted text and qualified-read result remain rooted through callbacks. Newly
created named function objects survive a script setter which collects and invokes
the function before publication finishes. Traced owned strings with NUL survive
name-coercion GC. Getter/setter exceptions and allocation failures restore roots;
ordinary fresh-cell injection checks no publication on failure. Callback side
effects are not rolled back. Root positions, modes and interpreter declaration
operand counts use documented uppercase macros in scoped headers.

Reference rules:
https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs
https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/scope.rs

Объявления и именованные функции подключены к единому C-обработчику. Различаются
собственные/наследуемые свойства With, наличие инициализатора и локальные/верхние
составные имена. Проверены GC при преобразовании имени и в setter новой функции,
существующий undefined, исключения, строки с NUL и отказы памяти. Числовые позиции
и режимы названы макросами. Полная семантика прототипов/регистра и резервного
поиска именованных функций, JIT и стадии 1–9 остаются открытыми.

Named-call cached fallback, complete qualified call receiver selection and full
Flash equivalence remain open. Passing these scenarios does not close a roadmap
stage or implement native code generation.

Evidence: 497/497 Linux AGG and 497/497 Cairo tests; one AGG executable-permission
launch failure passed after permission restoration. Thirteen selected heap/global
ASan + UBSan checks passed (host stack/leak coverage disabled). All 26 unchanged
original SWF<=8 traces matched in each profile. Fixtures include new-function GC
inside a setter and qualified uninitialized getter/exception behavior.
Проверено по 497 тестов AGG/Cairo, 13 выбранных проверок памяти и 26 исходных
эталонов каждого профиля. Полная совместимость и стадии роудмапа ещё открыты.
