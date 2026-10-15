# Missing binary operands / Недостающие бинарные операнды — v0.301

Audited loaded AVM1 binary operations now supply undefined for missing stack
values, then invoke the existing value-based C execution service. With one value
already on the stack it remains the right operand; undefined is inserted below
it as the left operand. With no values both operands are undefined. Sufficient
stacks and host-only strict underflow behavior are unchanged.

Coverage includes legacy arithmetic/equality/ordering/logical operations,
StringAdd and explicit string comparisons, Add2/Equals2/Less2/Greater/StrictEquals,
remainder and the six bitwise/shift operations: 24 Action identifiers. This is
not a general normalization of every Action: member, call, control-flow and other
operand policies still require separate audits. No dispatch identifier is added.

Representative raw-fixture results for a single existing numeric right operand 7:

| Operation | SWF4–6 | SWF7–8 |
| --- | --- | --- |
| Subtract | -7 | NaN |
| Add / Add2 | 7 | NaN |
| StringAdd | "7" | "undefined7" |
| Less2 | true | undefined |
| Greater | false | undefined |
| BitOr / BitXor | 7 | 7 |

With both values absent, SWF4 Divide retains its existing #ERROR# result; later
versions produce NaN. Equals2 and StrictEquals compare two undefined values as
equal. Existing per-version coercion and callback order are preserved.

[GC:] Operand-vector growth completes before existing values are shifted right.
The active interpreter frame already roots the vector. No script or collection
runs during the shift/undefined initialization, and bad_alloc propagates to the
existing execution containment scope. The public C services retain their own
roots and still reject missing pointers. No bytecode/stack arguments are added
to the C API intended for native/generated bodies.

The pinned comparison source remains Ruffle
2372a4e148a67d68efd796f4477b3cddd4f880cd: runtime.rs returns undefined for an empty
pop; activation.rs pops right then left for the audited binary actions. This is
a source comparison, not an independent Flash Player certification.

avm1_binary_missing uses raw Action bytes and independent expected values for
all 24 operations, zero/one existing values, host mode and SWF4–8. A scripted
right-hand valueOf collects during subtraction, checking operand position and
rooting. The allocation fixture now covers unary, empty binary and one-value
binary preparation, with GC and execution/root-scope checks between retries.
The existing loaded string fixture is updated for its changed binary underflow
expectations; raw byte values remain intact.

Загруженный AVM1 подставляет undefined вместо недостающих бинарных операндов.
Имеющееся значение остаётся правым аргументом; это проверено на некоммутативном
вычитании и на valueOf со сборкой мусора. Тесты охватывают 24 Action-кода,
SWF4–8, пустой стек и одно значение. Строгий host-режим сохраняется. Общие
C-обработчики, порядок преобразований и ABI не изменены; остальные операции
со стеком требуют отдельного аудита. Все этапы роудмапа остаются открытыми.

Validation: Linux AGG and Cairo each passed 584 tests after repairing launch
permissions for five AGG and two Cairo executables. Three affected heap/global
ASan+UBSan fixtures passed (no stack/leak coverage). All 26 accepted original SWFs
matched the oracle in both profiles. Initial new-fixture expectations for NaN
relational results were corrected from false to undefined; a host-only fixture
was kept strict after confirming it does not load a SWF. First failed logs remain
available; these were fixture corrections, not suppressed sanitizer failures.
