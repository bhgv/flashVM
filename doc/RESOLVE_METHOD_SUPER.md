# Resolver-returned method super / Super метода из __resolve — v0.505

Loaded SWF4–8 methods returned by __resolve execute with explicit superclass depth
one for ordinary member calls. For an explicit super facade, the invocation uses
its captured depth plus one. Resolver function execution itself keeps depth zero
(v0.504). The invocation still has no defining property owner; callback storage
and prototype mutation inside the resolver do not change the selected depth.
The facade depth is captured before resolver execution, with the existing private
prototype bound checked before incrementing. Selected ordinary stored methods,
host dispatch, resolver discovery and noncallable/error policy are unchanged.
The shared AVM1 helper applies depth one whenever loaded member lookup returns
a Function without a stored owner, including an extension metatable fallback;
the existing C API metatable-dispatch contract is unchanged.

[GC:] Existing call scopes retain callback identities and restore previous depth.
The selected lookup graph remains rooted during resolution; the completed method
is retained during invocation even after resolver deletion/prototype detachment.
No public ABI or graph wire format changes.

В загруженных SWF4–8 метод, возвращённый __resolve, получает super с глубиной 1.
При вызове через явный фасад super используется глубина исходного фасада плюс 1;
она сохраняется до выполнения resolver. Сам resolver по-прежнему вызывается с
глубиной 0. Изменение прототипов и удаление resolver внутри обработчика не меняют
выбранную глубину и не лишают активный метод его корней GC. Контекст вызывающей
функции восстанавливается при успехе, исключениях и ошибках выделения памяти.

Tests: resolve_method_super reproduces the pre-fix failure and covers SWF4–8,
regular functions, DefineFunction2 (SWF7–8), ordinary calls and explicit super
bases 0–3, counted-NUL missing names, C/interpreter/explicit/automatic JIT routes,
public ancestor reads and GC. resolve_method_super_roots deletes the resolver,
detaches the receiver prototype, collects in resolver and returned method, checks
capture roots and outer-depth restoration on success/throw/error, including
explicit super. resolve_method_super_allocation sweeps new/new[] failures for
missing-prototype receivers and retries. resolve_method_super_transport restores
ordinary receivers and super facades with pure AVM1 resolvers/functions, then
invokes and collects in another loaded context.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
object.rs search_prototype returns resolver results at depth 0; call_method uses
depth.max(1). super_object.rs call_method uses facade.depth + lookup.depth + 1.
Source comparison is not an original Flash Player oracle. General stored-method
and other callback depth rules, complete SWF compatibility and roadmap stages
remain open.

Validation / Проверка: Linux x64 C++98 full minimal 1026, AGG 1045 and Cairo
1046 tests passed after restoring execute permission on 3/6/9 generated binaries
and rerunning failed launches. Eight targeted ASan/UBSan tests passed. A strict
C89 depth-header probe and 26 Unicorn branch/error cases across x86, ARM, ARM64,
RV32 and RV64 passed; native x64 ran in host suites. Foreign operating systems
and full SWF compatibility remain unverified.
