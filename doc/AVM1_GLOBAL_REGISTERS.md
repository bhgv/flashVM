# Shared global values / Общие регистровые значения

v0.238 stores the four legacy global registers in their owning context. Loaded
root actions, ordinary functions and Function2 with zero private registers read
and write that same bank across activations. Positive private Function2 banks
remain separate. Unloaded host bytecode retains its previous isolated registers.
Global state is not copied by function graph transport; each receiving context
owns its own bank.

В v0.238 четыре общих регистра принадлежат контексту. Загруженные действия,
обычные функции и Function2 без частного набора используют их между вызовами.
Частные наборы Function2 остаются отдельными. GC сохраняет объекты, метатаблицы
и строки в общих регистрах до перезаписи; новый контекст начинает с undefined.

`fvm_exec_global_register` is a C89 value-based service for generated bodies and
host code. Scalar/object stores require no allocation. String stores adopt the
bytes before publishing the retained value, preserve metatables, and leave the
old slot and output untouched on allocation failure. Input/output aliasing is
supported. Missing slots return undefined and discard validated writes.

A prepared native frame exposes the live bank for these loaded activations.
Direct register writes must respect context ownership and retained-value
lifetimes; use the execution service to adopt borrowed string bytes. The public
frame layout and graph layout are unchanged. Slot count uses the documented
FVM_EXEC_GLOBAL_REGISTER_COUNT macro. Context initialization and GC root marking
include every slot.

Tests cross loaded SWF4–8 and unloaded host contexts, ordinary/zero/private
Function2 functions, native attachment/unbinding, context isolation, mutable
borrowed strings with embedded NUL, metatable/object retention and collection
following overwrite. Separate C89 and allocation-failure fixtures exercise the
public service. Exact private register-count boundaries remain follow-up work.
No roadmap stage, JIT backend or non-Linux acceptance is declared complete.

Validation: Linux AGG and Cairo Release each pass 442 tests. Cairo required
eight execute-bit launch retries, which passed after restoring permissions.
Six focused heap/global ASan+UBSan checks pass with stack/leak instrumentation
disabled. All 23 original unchanged SWFs match byte for byte with zero exit and
empty stderr in each graphics profile. The added C fixture compiles as C89.
