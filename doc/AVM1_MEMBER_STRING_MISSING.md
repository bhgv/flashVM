# Missing member/string operands / Недостающие значения членов и строк — v0.303

Loaded StringExtract/MBStringExtract, GetMember/SetMember and Delete now prepare
missing operands as undefined below the existing top values before invoking the
shared C service. Host-only execution remains strict. Operation-specific coercion,
primitive boxing and member/deletion semantics are still handled by the existing
value-based functions; no bytecode or stack argument enters their public API.

| Existing values before operation | Loaded interpretation/result |
| --- | --- |
| StringExtract with no values | Empty result in SWF4–8 |
| StringExtract with length 7 only | Empty in SWF4–6; "undefin" in SWF7–8 |
| StringExtract with start 2 and length 3 | Empty in SWF4–6; "nde" in SWF7–8 |
| GetMember with a single name | Read that name from a temporary undefined-owner wrapper |
| SetMember with name and value only | Assign to the temporary undefined-owner wrapper, then discard it |
| Delete with a single name | Convert the name and return false for undefined owner |

The string results follow the existing versioned undefined-to-string conversion:
empty before SWF7, the word undefined from SWF7 onward. Both extract actions retain
the current ASCII-only legacy string boundary; historical locale/multibyte
semantics are not expanded. Other member/prototype/boxing policy gaps remain open.

[GC:] The active operand root vector grows before shifting existing values to
preserve their top-relative roles. Shared C services retain original/converted
operands through callbacks. The name-conversion regression deletes the original
global bindings and collects, including an object-valued SetMember assignment;
execution must retain operands until the temporary write finishes. Root-chain
restoration is checked after every call. Allocation injection now covers empty,
one-value and two-value three-operand preparation with GC between retries.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs (the five affected actions) and runtime.rs (missing
pop). This source comparison and raw-byte fixture do not certify full Flash Player
compatibility or complete missing-stack coverage. No opcode reference is added.

avm1_member_string_missing covers host mode and SWF4–8, every missing count for
these operations, independent expected string results and collecting key coercion.
The existing loaded string fixture's final underflow checks now expect successful
execution for all its audited actions; its original raw bytes are preserved.

Пять Action-кодов теперь подставляют undefined ниже имеющихся значений и передают
операнды общим C-функциям. Начало и длина извлечения не меняются местами; одиночное
значение GetMember остаётся именем свойства. Преобразование имени удаляет исходные
глобальные ссылки и вызывает GC, проверяя сохранность операндов. Проверены SWF4–8,
все варианты недостающих значений, строгий host-режим и отказы памяти. Прежние
ограничения строк и объектов сохраняются; все стадии роудмапа остаются открытыми.

Validation: Linux AGG and Cairo each passed 586 tests after repairing seven AGG
and four Cairo launch-permission failures. Both targeted heap/global ASan+UBSan
fixtures passed; the strengthened binding-dropping callback fixture was rebuilt
and passed in all three profiles. All 26 accepted original SWFs matched the
pinned oracle in each renderer profile. Dispatch references remain 94/100.
