# Missing call operands / Недостающие операнды вызова — v0.309

Loaded CallFunction/NewObject/CallMethod/NewMethod now initialize missing fixed
operands as undefined in the interpreter's rooted operand vector. Existing top
values retain their roles: a lone named-call operand is the name, while a lone
method operand is the method name and its receiver is missing. Two method
operands retain receiver and name, with an undefined argument count. Missing
counts convert to zero through the existing C count service. Host underflow
remains strict; public C services still require valid explicit input pointers.

Named calls still convert the name before the count; method calls convert the
count and skip nullish receivers before converting the method name. The change
does not synthesize variable arguments or change existing count clamping, scope
lookup, receiver selection, construction, or public ABI. No bytecode/operand-stack
dependencies are introduced into the shared C handlers.

avm1_call_missing tests raw Action bytes in host mode and SWF4–8: empty and partial
layouts, zero-argument function/constructor invocation, skipped method-name
conversion on missing receivers, and native bodies that collect and throw.
avm1_call_missing_allocation injects failures through ten partial call layouts in
each loaded version and collects between attempts. Root/execution chains must be
restored on success and failure. Heap/global ASan+UBSan coverage excludes stack
and leak instrumentation.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs action_call_function/action_new_object/
action_call_method/action_new_method and runtime.rs missing-pop policy.
This is bounded operand-preparation coverage, not complete call compatibility.

В загруженных SWF4–8 четыре операции вызова подставляют undefined в недостающие
фиксированные позиции. Существующее верхнее значение сохраняет назначение;
недостающее количество аргументов становится нулём. Проверены вызовы функций и
конструкторов, пропуск преобразования имени при отсутствующем объекте, GC внутри
нативного тела, исключения и отказы памяти. Host-режим остаётся строгим. Общие
C-обработчики сохраняют явные параметры и пригодны для будущего JIT; сам JIT и
все стадии роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 597 tests; two AGG executable-permission
launch failures passed after restoring permissions. Both targeted heap/global
ASan+UBSan tests passed. All 26 accepted original SWFs matched the pinned oracle
in both profiles. The source Action-reference audit remains 94/100.
