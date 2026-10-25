# AVM1 execution API

FlashVM implements an AVM1 subset in C++98 with a C89 embedding API.
It is not a complete Flash 8 runtime. FWS/CWS loading through SWF8, GC,
prototypes, metatables, registered native methods and named script functions
are available; display/timeline and JIT coverage remains incomplete.

[`fvm_avm1_execute`](../include/flashvm/avm1.h#L46) validates a complete action stream ending in End and executes
it in the context. [`fvm_swf_execute_actions`](../include/flashvm/avm1.h#L93) executes top-level DoAction tags of
the loaded canonical FWS, not a full timeline/sprite event loop.
[`fvm_avm1_call`](../include/flashvm/avm1.h#L82) invokes a named function with rooted parameters and receiver.
Native CallFunction callbacks, globals and AS properties share the context.

Tests under `tests/avm1*`, `tests/interpreter.c`, `tests/swf_execute.c` and
`tests/actionscript*` describe supported operands and instructions: primitive
Push values and constants, stack operations, variables, local definitions,
properties, arithmetic, branches, native/script calls and returns. Unsupported
instructions return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). Branches stay within the current action
block and must target parsed instruction boundaries. Each function has its own
instruction budget; nested try/catch/finally blocks share that budget.

SWF7–8 ActionTry and ActionThrow support catch by name/register, propagation
through function calls, finally on return and finally overrides. See
[exception API and limits](EXCEPTIONS.md). Existing DefineFunction framing and
four-register execution remain limited; DefineFunction2 is unsupported.

The context and host APIs must not execute concurrently on one context. VM
failure does not roll back globals already changed. Native allocation failures
and unsupported bytecode are statuses, not AS exceptions. This checkpoint does
not establish conformance or safety for arbitrary untrusted SWF.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_avm1_call` | function | [include/flashvm/avm1.h:82](../include/flashvm/avm1.h#L82) |
| `fvm_avm1_execute` | function | [include/flashvm/avm1.h:46](../include/flashvm/avm1.h#L46) |
| `fvm_swf_execute_actions` | function | [include/flashvm/avm1.h:93](../include/flashvm/avm1.h#L93) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
