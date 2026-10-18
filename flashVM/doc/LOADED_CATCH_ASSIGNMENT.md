# Loaded catch assignment / Присваивание имени catch в загруженном SWF

v0.261 makes named catch binding in loaded SWF4–8 use the common variable-write
service, matching the pinned Ruffle ActionTry/Activation::from_action policy.
The caught value is assigned through the current scopes/target; the catch block
shares that scope rather than introducing a private restored binding. Existing
local/With bindings are updated, missing names use normal target/global fallback,
and qualified names use the normal write path. The previous value is not restored
after catch, so finally sees the assigned value. Register catches keep the shared
register-bank behavior.

Unloaded host execution keeps its prior temporary/restored catch policy using the
managed activation introduced in v0.260. Its exported closure retains local data
across GC/channel transfer. Loaded top-level functions instead use their ordinary
global namespace, including the destination context's globals after transfer.
This supersedes the loaded temporary-binding behavior recorded in v0.260.

[GC:] The caught object is retained in the saved thrown-value slot after clearing
the pending exception. The shared write service then retains it through prefix
getters and script setters. A catch-variable setter can collect; a new setter
exception propagates before executing the handler/finally body, following the
reference implementation's early-error path. Tests check preserved thrown-object
fields and restored setter/native/script/root metadata. Allocation failures check
lexical restoration. With/host-catch publication now uses one noncopyable scoped
C++ guard, retaining the existing C89 external execution interfaces.

Reference source:
https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs
The unchanged SWF8 catch_references_registers probe (one declared frame) is added
to the external corpus; its trace already matched before this change. The modern
try_catch_finally, try_catch_stack and try_finally_simple originals are SWF15 and
remain excluded, without altering headers or expected outputs. New assignment
fixtures are source-based checks, not a replacement Flash Player oracle.

В загруженном SWF имя catch присваивается через обычные области/цель, включая
With, существующий локальный параметр и составное имя. Прежнее значение после
catch больше не восстанавливается; finally видит новое. Режим без загруженного
SWF сохраняет прежнюю временную привязку. Проверены GC в setter пойманного
объекта, исключение при самом присваивании, каналы и восстановление состояния.

Full this-cell mutation, legacy non-ASCII bytecode decoding/property casing,
catch/base-target corner cases and complete Flash equivalence remain open.
These scenarios do not close roadmap stages or certify the JIT/platform gates.

Evidence: 493/493 Linux AGG and 493/493 Cairo tests; three AGG execute-bit
launch failures passed after restoring permissions. Twelve selected heap/global
ASan + UBSan checks passed, with stack/leak coverage disabled on this host.
All 26 unchanged original SWF<=8 traces matched in both profiles. Added fixtures
cover persistent local/global/qualified/With assignment, getter-free setter entry,
GC in the catch-variable setter and a replacement exception before handler/finally.
Проверено по 493 теста AGG/Cairo, 12 выбранных проверок памяти и 26 исходных
эталонов в каждом профиле. Полная совместимость catch остаётся открытой.
