# Named call names / Имена именованных вызовов

v0.245 converts loaded CallFunction and NewObject names using the version-aware
C89 service [`fvm_exec_to_string`](../include/flashvm/execution.h#L634). Conversion runs before the argument-count
callback, and the resulting text replaces the rooted stack operand before count
conversion can collect. Numeric and object names now participate in ordinary
counted name lookup. Unloaded hosts retain their string-only action contract.

CallMethod name conversion and the ToString instruction also use this service.
It accepts values rather than instructions/stacks; outputs may alias inputs,
and conversion errors preserve the result. Existing string values retain their
ordinary borrowed ownership contract. No new public ABI version is implied.

[GC:] The input is temporarily rooted through toString callbacks. The caller
retains the resulting counted text before any subsequent callback/collection.
The method service retains its primitive box and converted name in dedicated
slots. New tests check name/count/callee callback ordering, collection, exceptions,
numeric names, construction and SWF4–8. C89 tests cover Boolean/undefined version
spelling, aliasing and root-allocation failure recovery.

Primary comparison: Ruffle commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
core/src/avm1/activation.rs, action_call_function/action_new_object. These source
comparisons do not establish independent Flash Player conformance. Full missing/
noncallable binding, general case policy and broader scope semantics stay open.

Загруженные CallFunction/NewObject преобразуют имя до числа аргументов.
Полученная строка сохраняется в корнях до следующего callback. Общая C-служба
используется также ToString и вызовом метода; она не зависит от байткода или
стека. Проверены порядок преобразований, числовые/объектные имена, GC,
исключения, создание объектов и прямой C89-вызов. Полная семантика поиска
и все стадии роудмапа остаются открытыми.

Evidence: 452 Linux AGG/Cairo tests pass per profile after execute-bit recovery
for one AGG and five Cairo launch failures. Nine focused heap/global ASan+UBSan
checks pass; stack/leak instrumentation is unavailable. All 23 unchanged original
SWF traces remain byte-exact per profile. No JIT/non-Linux completion is claimed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_to_string` | function | [include/flashvm/execution.h:634](../include/flashvm/execution.h#L634) |
<!-- source-links:end -->
