# Missing stack/control operands / Недостающие значения управления — v0.302

Loaded bytecode now applies the audited missing-pop policy to seven additional
Action identifiers: Pop, PushDuplicate, StackSwap, Return, Throw, If and Trace.
Host-only execution keeps its strict underflow errors. Explicit value-based C
service validation and malformed encoded-branch validation are unchanged.

| Loaded empty-stack operation | Behavior |
| --- | --- |
| Pop | Continue without allocating an operand |
| PushDuplicate | Leave two undefined values |
| StackSwap | Leave two undefined values |
| Return | Explicitly return undefined |
| Throw | Raise an ordinary pending undefined exception |
| If | Convert undefined to false and continue without taking the branch |
| Trace | Deliver the literal text "undefined", including SWF4–6 |

With one existing numeric value 7, StackSwap leaves 7 below undefined: the missing
lower value is prepared before applying the normal exchange. Encoded If payload
length is validated before supplying the missing condition. This does not change
other target/member/call/initializer stack policies or close AVM1 compatibility.

[GC:] Value-consuming operations reuse the already rooted interpreter operand
preparation. Empty Pop is a direct no-op and introduces no allocation. Return
uses the established activation unwinding; Throw uses the existing pending-value
root, and trace uses the existing host native-callback barrier. No public ABI,
resource identity, Action encoding or serialized callback identifier changes.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/runtime.rs and activation.rs. Its missing pop returns undefined;
the audited actions then perform their ordinary operations. Trace explicitly
special-cases undefined to print the literal word before legacy string coercion.
This is bounded source-based regression evidence, not full player certification.

avm1_control_missing uses raw bytes in host and SWF4–8 modes, verifies duplicate
and swap output order, explicit function return, pending thrown undefined,
untaken conditional continuation, malformed branch payloads and the trace sink.
The shared preparation allocation fixture supplies additional failure/root-scope
coverage. Heap/global ASan+UBSan coverage excludes stack and leak instrumentation.

Загруженный SWF теперь обрабатывает отсутствие значения в семи простых стековых
и управляющих операциях по общей политике undefined. Проверены порядок результатов
Swap, возврат из функции, сохранённое исключение undefined, ложное условие If и
текст Trace. Пустой Pop ничего не выделяет. Прямой host-режим остаётся строгим;
проверка повреждённого байткода и публичный C API не меняются. Все стадии открыты.

Validation: Linux AGG/Cairo each passed 585 tests; four AGG launch-permission
failures were repaired and passed on retry, while Cairo completed directly.
Both targeted heap/global ASan+UBSan fixtures passed. All 26 accepted original
SWFs matched the pinned oracle in both profiles. No semantic test assertions
failed in these full regression runs; stage completion is not implied.
