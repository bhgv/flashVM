# Missing variable/target operands / Недостающие имена переменных и цели — v0.304

Loaded GetVariable, SetVariable, Delete2, DefineLocal, DefineLocal2 and SetTarget2
now supply undefined for missing stack operands before calling their existing C
execution services. SetVariable and DefineLocal preserve a lone existing value
as the assigned value, supplying the missing name below it. Host-only execution
retains its stack errors and GetVariable/SetVariable string-name requirement.

SetTarget2 follows its existing explicit-undefined version policy even when no
operand is present: SWF4–6 resets the selected target to the activation's base;
SWF7–8 invalidates the selected target. This change does not implement removed
clip/descendant semantics or replace the activation's current target model.

Variable names use the existing versioned undefined conversion: an empty name in
SWF4–6, the word undefined in SWF7–8. Existing empty-name assignment/declaration
no-op rules remain intact. Loaded name lookup/deletion and declaration retention
continue through the shared managed-scope services, without a new local-map API.
No public C ABI, graph schema or Action identifier is added.

avm1_variable_target_missing uses raw bytes in host and SWF4–8 modes. It checks
lookup, assignment with zero/one values, initialized/uninitialized declaration,
and deletion on an explicit root clip. SWF7–8 uses a real own binding named
undefined, avoiding reliance on the standard global's built-in property flags.
The existing empty-name policy yields no binding in older loaded modes.

The target test first selects a child, then executes SetTarget2 with an empty
stack. A synchronous trace probe observes the base/current target while the
activation is still live, checking reset versus invalidation directly. After
execution, the caller's inactive base/target state must be restored. Existing
loaded target-scope underflow expectations are updated; malformed encoded
SetTarget validation remains unchanged. Raw fixture bytes are preserved.

[GC:] Preparation uses the active operand-vector roots. C services keep their
own scope/name/value roots and allocation containment. Target state is published
through the existing context roots; activation scope restores the suspended
caller's target state on exit. The variable set/declaration and scope-delete
allocation fixtures accompany the new raw-byte regression fixture.

Primary source comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs and runtime.rs. Audited actions consume missing pops
as undefined; SetTarget2 has an explicit SWF6/SWF7 boundary. Source comparison
and bounded fixtures do not certify full historical scope/target semantics.

Шесть операций загруженного SWF теперь получают undefined вместо отсутствующего
имени или цели. Одиночное значение SetVariable/DefineLocal сохраняется как значение
присваивания. Для SetTarget2 проверена граница SWF6/SWF7 непосредственно внутри
активного исполнения: сброс к базовому клипу либо недействительная текущая цель.
После выхода восстанавливается состояние вызывающего кода. Пустые имена сохраняют
прежние правила общих C-служб; host-режим остаётся строгим. Полная семантика областей,
удалённых клипов и все стадии роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 587 tests after restoring executable
permissions for eight AGG and two Cairo launch failures. Four targeted heap/global
ASan+UBSan fixtures passed (no stack/leak coverage). All 26 accepted original SWFs
matched the pinned oracle in both profiles. The first new fixture incorrectly
assumed the existing loaded property API creates an empty-name binding; its setup
expectation was corrected after checking that empty-name writes are ignored.
A nonexistent sanitizer target name was corrected to the actual scope-delete
allocation fixture before successful validation; no stale executable was used.
