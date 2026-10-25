# Extends service / Общая операция наследования — v0.285

C89 `fvm_exec_extends(context, subclass, superclass)` accepts ordinary values;
it does not read bytecode or the interpreter stack. The interpreter only locates
its two operands and consumes them after success. A future native backend can
call the same service with the same values and result/error convention.

Loaded SWF execution boxes the superclass before the subclass, using the current
primitive constructors. A missing/nullish result becomes a bare object. After
both conversions, ordinary `superclass.prototype` lookup invokes its getter.
A fresh object receives own `__proto__` data, plus own nonenumerable `constructor`
and `__constructor__` references to the prepared superclass. Defining these
fields does not invoke inherited setters. Final subclass.prototype assignment
uses ordinary property dispatch, including its setter. Self-extension and
noncallable class objects are accepted. Primitive prototype data, including
counted strings with embedded NUL, is preserved without creating an object link.
Host-only use retains the existing strict [`fvm_class_extend`](../src/standard/inheritance.cpp#L104) policy.

[GC:] Original operands, boxed receivers, getter results and the new prototype
stay rooted through callbacks. Exceptions propagate with roots restored; user
callback effects remain visible. Preparation allocation failures do not publish
a partially built prototype. The final property setter keeps its existing
failure/side-effect policy.

Primary semantic reference: Ruffle's
[action_extends implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
This is a pinned implementation comparison, not a claim of complete historical
player equivalence. Full property-name folding and roadmap acceptance remain open.

Общая C-функция отделена от стека и Action-кодов. Сначала упаковывается superclass,
затем subclass, после чего выполняется getter prototype. Новый прототип получает
собственные поля без вызова наследуемых setter; окончательное присваивание
subclass.prototype сохраняет обычный setter. Корни GC удерживают промежуточные
значения при сборке мусора и исключениях. Побочные эффекты пользовательских
обработчиков не откатываются. Отдельные проверки охватывают SWF 4–8, прямой вызов
C, Action Extends, упаковку, скалярные прототипы, исключения и отказы памяти.
Машинный JIT и завершение стадий 1–9 этим изменением не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_class_extend` | function | [src/standard/inheritance.cpp:104](../src/standard/inheritance.cpp#L104) |
| `fvm_exec_extends` | function | [include/flashvm/execution.h:291](../include/flashvm/execution.h#L291) |
<!-- source-links:end -->
