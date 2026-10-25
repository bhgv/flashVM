# Common plain variable lookup / Общий поиск обычных имён

v0.260 moves plain variable lookup and historical this-name matching out of the
interpreter into [`fvm_exec_variable_lookup`](../include/flashvm/execution.h#L552). Explicit managed scope, target, this,
default callable receiver and a local-scope flag replace raw local-map access.
The interpreter bridge selects its current display/default receivers and forwards
the counted key. Qualified paths remain the separate shared variable-path phase.

Loaded SWF5+ implicit this precedence, SWF5 local/global casing distinctions,
SWF6 counted UCS2 folding and SWF7+ exact spelling preserve the former interpreter
policy. Activation/global bindings keep the supplied default call receiver;
With bindings use their wrapped object and target bindings use their target.
Missing global fallback and present undefined retain separate found results.
Top-level catch maps now use a managed temporary activation, so ordinary reads,
GC and exported closures share the same lexical model. Existing named catch
binding restoration is preserved here; complete loaded catch-assignment semantics
remain open. Saved Try/catch/finally slots now use documented root macros.

The existing [`fvm_exec_scope_lookup`](../include/flashvm/execution.h#L468) API delegates to the class-aware resolver,
[`fvm_exec_scope_resolve`](../include/flashvm/execution.h#L476), without changing its signature.

The binding's With classification is captured before invoking its getter. A
getter may detach the scope chain and collect. The previous interpreter rescanned
the changed chain after the getter, potentially losing the original With receiver
and calling a function with the global/default receiver. The new resolver returns
the captured class with the value/owner. A direct C-service test permits the
unlinked wrapper to be collected; a nested interpreter With/CallFunction fixture
checks the actual callee receiver after detachment and GC.

[GC:] Scope, target, explicit this/default receiver, original name and unpublished
result remain rooted through getters. C scope resolution captures class metadata
before callbacks and does not access a detached wrapper after getter return.
Ordinary native getters retain their collector barrier; script getters may collect.
Errors preserve result/receiver/presence metadata, and allocation injection checks
restoration of the caller's root chain. Layout/case-table column constants use
scoped documented uppercase macros. The pinned case table itself is unchanged.

Поиск обычного имени и определение получателя вызова вынесены из interpreter.cpp.
Сохранены правила this по версиям и разделение undefined/отсутствия. Класс With
фиксируется до getter: изменение цепочки и GC больше не подменяют получателя
вызова. Старый C-интерфейс scope_lookup сохранён как оболочка. Новые проверки
покрывают фактический вызов, C-службу, GC, исключения, алиасы и отказ памяти.

This preserves the verified implicit-this policy, not complete inherited/mutable
this-cell conformance. Full legacy property casing, qualified CallFunction routing,
removed-target/catch semantics and general Flash equivalence remain open. All roadmap
stages remain open; these C services are callable foundations for future JIT code.

Evidence: the final run passes 491/491 Linux AGG and 491/491 Cairo checks; two
AGG execute-bit launch failures passed after permission restoration. Seventeen
selected heap/global ASan + UBSan checks pass (host stack/leak coverage disabled).
All 25 unchanged reference SWFs match per profile. Initial top-level catch read
regressions were fixed with managed catch scope publication before this final run;
new catch fixtures cover GC, closure/channel transfer and OOM lexical restoration.
Проверено по 491 тесту AGG/Cairo, 17 выбранных проверок памяти и 25 эталонов в
каждом профиле. Полная семантика catch и исторических областей ещё не заявляется.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_scope_lookup` | function | [include/flashvm/execution.h:468](../include/flashvm/execution.h#L468) |
| `fvm_exec_scope_resolve` | function | [include/flashvm/execution.h:476](../include/flashvm/execution.h#L476) |
| `fvm_exec_variable_lookup` | function | [include/flashvm/execution.h:552](../include/flashvm/execution.h#L552) |
<!-- source-links:end -->
