# Anonymous CallMethod super / Super анонимного CallMethod — v0.509

Loaded SWF4–8 nonconstructing CallMethod with an empty or undefined name invokes
the Function value with explicit superclass depth zero and no defining member
owner. Function storage does not determine this depth, and missing prototypes
still yield a facade with exhausted reads returning undefined. The existing
anonymous this binding is preserved. A callable super facade still establishes
its own constructor scope and relative depth inside the anonymous call.

[GC:] The invocation scope retains the active Function and restores prior
constructor/function/owner/depth metadata. Existing execution roots retain
operands and arguments through callback removal, GC, throw, error and allocation
failure. Unloaded host inference, named member calls, NewMethod, public direct
fvm_function_invoke and CallFunction/qualified-name policy are unchanged. No
public ABI or graph format changes.

В загруженных SWF4–8 CallMethod с пустым или undefined именем вызывает значение
Function с глубиной super 0, независимо от места хранения функции. Сохранена
привязка анонимного this. Отсутствующий прототип не удаляет фасад super; вызов
самого фасада по-прежнему устанавливает отдельный контекст конструктора.
Корни и прежний контекст восстанавливаются при GC, исключениях и ошибках.
Незагруженный host, именованные методы и публичный прямой вызов Function не меняются.

Tests: anonymous_call_super reproduces the pre-fix failure and checks SWF4–8,
regular functions, DefineFunction2 (SWF7–8), preload/suppress combinations,
empty/undefined names, independent function storage, deletion of temporary global
bindings, absent/short chains and C/interpreter/explicit/automatic JIT routes.
anonymous_call_super_roots removes the last function property, detaches the
receiver prototype, collects, checks capture roots and restores an outer explicit
depth on success/throw/error; includes an unloaded host control.
anonymous_call_super_allocation sweeps new/new[] failure/retry with traced counted
arguments and a missing prototype. anonymous_call_super_transport restores a pure
AVM1 Function into another loaded context and checks that its facade refers to
the receiving context's anonymous this after binding deletion and GC.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
function.rs Executable::call invokes exec with depth 0. This source comparison
is not an original Flash Player oracle. Other direct/qualified function-call
rules, complete SWF compatibility and all roadmap stages remain open.

Validation / Проверка: full Linux x64 C++98 minimal 1042, AGG 1061 and Cairo
1062 tests passed. Minimal/AGG required execute-permission repairs on 5/3 generated
binaries and reruns; Cairo passed directly. Eight targeted ASan/UBSan tests,
a strict C89 depth-header probe and 26 Unicorn branch/error cases across x86,
ARM, ARM64, RV32 and RV64 passed. Native x64 ran in host suites. Foreign-OS
acceptance and complete SWF compatibility remain open.
