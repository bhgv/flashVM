# Missing class operands / Недостающие операнды классов — v0.307

Loaded Extends, CastOp and InstanceOf now fill missing stack operands with
undefined below existing top values, then execute the existing shared C service.
Host-only missing-stack errors remain strict. ImplementsOp and other variable
argument-count policies are not changed by this checkpoint.

| One existing value | Its preserved role |
| --- | --- |
| CastOp | Value being cast; missing constructor is undefined |
| InstanceOf | Constructor operand; missing candidate is undefined |
| Extends | Superclass; missing subclass is undefined |

Zero/one-value primitive fixtures produce null for CastOp, false for InstanceOf,
and successful completion for loaded Extends under its existing boxing policy.
The change does not bypass primitive constructor effects or superclass prototype
getters merely because another operand is missing.

The CastOp regression replaces Number with a script-backed constructor. A single
existing numeric value 7 must still invoke it once with argument 7; the body
collects and can throw. Extends receives a single superclass whose prototype is
a script-backed getter. The getter deletes its global binding and its own
accessor, collects, then returns its captured prototype or raises an exception.
This distinguishes superclass from subclass alignment and verifies propagation.

[GC:] Prepared values remain in the active operand-vector roots. Existing C
services retain boxing results, detached getters/prototypes and captures through
callbacks; exception unwinding restores root/execution chains. The new raw-byte
fixture checks those chains after success and failure. Existing service-specific
allocation injection accompanies the regression fixture in sanitizer coverage.

avm1_class_missing covers host mode and loaded SWF4–8 with raw Action bytes,
empty and one-value stacks, collecting/throwing boxing and prototype accessors.
The comparator source is pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/runtime.rs and activation.rs (CastOp, InstanceOf, Extends). It returns
undefined for missing pops and preserves each action's operand order. This is
bounded evidence, not full prototype/inheritance/player conformance.

No public C signature, serialized resource/callback identifier or Action-reference
count is changed. Full constructor/prototype/interface semantics, JIT backends
and all roadmap stages remain open.

Extends, CastOp и InstanceOf загруженного SWF теперь получают undefined вместо
недостающих операндов. Существующее верхнее значение сохраняет роль, указанную
в таблице. Проверены упаковка числа с вызовом заменённого Number, getter прототипа,
удаление исходных ссылок, GC и исключения. Политики общих C-служб и строгий
host-режим сохранены; ImplementsOp этим изменением не затронут. Публичный ABI
не меняется. Полная семантика наследования и все стадии остаются открытыми.

Validation: Linux AGG/Cairo each passed 593 tests after restoring executable
permissions for five AGG and two Cairo launch failures. Four targeted heap/global
ASan+UBSan fixtures passed (no stack/leak coverage). All 26 accepted original SWFs
matched the pinned oracle in each profile. An initial test build attempted to
nest two main-renaming fixtures; it was replaced with local callback helpers
before successful compilation and test execution. No source fixture bytes changed.
