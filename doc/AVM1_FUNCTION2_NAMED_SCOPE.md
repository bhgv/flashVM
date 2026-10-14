# Function2 inherited names / Унаследованные имена Function2

v0.218: in loaded Function2 calls, selecting either preload or suppression for
arguments/super leaves their named lookup in the enclosing lexical scope.
No local implicit property shadows that scope. Preloaded registers still contain
the current call's arguments/super projection, following paired-flag rules.
An explicit named formal parameter is installed afterwards and can shadow the
enclosing name. Unloaded host calls keep their existing implicit-binding behavior.
The special inherited this-cell model and complete native/no-owner super behavior
remain open; this change only covers arguments/super named bindings under flags.

В загруженном Function2 preload/suppress для arguments или super не создаёт
локальную неявную запись: поиск продолжает окружающую лексическую область.
Регистры содержат собственные значения текущего вызова по правилам парных флагов.
Явный параметр устанавливается позже и может затенить унаследованное имя.
Host-контекст сохраняет прежний контракт. this-cell, полная семантика super без
владельца и нативных суперклассов остаются открытыми.

Reference: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/function.rs load_arguments/load_super: local names are defined only
in the non-preloaded, nonsuppressed branch. `avm1_function2_named_scope` creates
escaped nested functions and tests flags 4, 8, 12, 16, 32, 48 across SWF4–8.
It checks enclosing arguments/super values, actual arguments registers and paired
suppressed super, explicit formal shadowing, post-return GC and channel transfer
followed by GC and invocation in another loaded context. No wire/ABI change.
This source-based fixture does not certify independent Flash Player conformance
or complete Function2/scope compatibility.

Тест проверяет перечисленные флаги и значения после возврата внешнего кадра,
GC и переноса функции с замыканием в другой контекст. Формат/ABI прежние.
Полная совместимость Function2 и независимая сверка Flash Player не заявляются.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 376 tests passed
(one execution-permission retry), Cairo Release 375 passed without retries.
Eight selected AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan;
stack/leak checks remain disabled. All 17 unchanged selected original SWF<=8
probes retain zero exits, empty stderr and byte-exact traces. No completed stage
or non-Linux runtime verification is claimed.
