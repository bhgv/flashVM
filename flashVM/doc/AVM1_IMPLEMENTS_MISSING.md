# Missing ImplementsOp operands / Недостающие операнды ImplementsOp — v0.308

Loaded ImplementsOp now prepares its missing fixed operands as undefined before
calling the existing count and declaration C services. The fixed pair remains
count below constructor. With zero or one existing values, the missing count
converts to zero and no interfaces are consumed or installed. Host-only stack
underflow stays strict. Explicit nonmissing counts and declaration semantics are
unchanged; this does not expand the historical interface policy.

The zero-count path must not mark a prototype's interface snapshot as installed,
replace a previous declaration or box a primitive constructor. The new raw-byte
fixture checks empty input, a lone numeric constructor and a lone object
constructor with a real prototype. A replaced Number constructor fails if called;
all three cases must finish without invoking it and without setting the prototype
snapshot marker or interface list. Normal execution continues after the action.

[GC:] Missing values are initialized in the active rooted operand vector. Existing
count conversion and declaration services retain their own operands and exception
containment; successful execution and allocation failures restore the caller's
root/execution chains. The failure fixture covers empty/single-value bytecode in
SWF4–8, with GC between attempts. Current zero-count service preparation can
allocate and report [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44); no allocation-free guarantee is introduced.

avm1_implements_missing covers host mode and SWF4–8 with raw Action bytes;
avm1_implements_missing_allocation injects every execution preparation allocation.
The existing interface snapshot allocation fixture supplies additional regression
coverage. Public C signatures, graph schemas, interface storage and Action IDs
remain unchanged.

Primary comparison: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs action_implements_op and runtime.rs missing-pop policy.
Its zero count is a no-op and primitive constructors are not boxed. That source
notes historical player differences; FlashVM retains the already documented
loaded policy rather than claiming full older-player interface conformance.

Загруженный ImplementsOp теперь подставляет undefined в недостающие фиксированные
позиции. При пустом стеке или одном значении недостающее количество становится
нулём: интерфейсы не устанавливаются, прототип не помечается как получивший список,
примитивный конструктор не упаковывается. Это проверено с заменённым Number,
который завершил бы тест ошибкой при вызове. Проверены SWF4–8, строгий host-режим,
отказы памяти и GC между попытками. Схема графа и общие C-службы не меняются;
исторические различия интерфейсов и все стадии роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 595 tests; two AGG launch permissions were
restored and passed on retry, while Cairo completed directly. Three targeted
heap/global ASan+UBSan fixtures passed (no stack/leak coverage). All 26 accepted
original SWFs matched the pinned oracle in each profile. The Action-reference
audit remains 94/100 and all stage-completion gates remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
