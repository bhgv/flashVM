# Watcher superclass depth / Глубина super обработчика watch — v0.502

Loaded SWF4–8 Object.watch callbacks receive explicit superclass depth zero,
independent of callback storage. A missing receiver prototype still produces a
super facade; exhausted reads return undefined. Regular AVM1 and DefineFunction2
preload/suppress handling use the same activation metadata. Unloaded host
callbacks retain owner-derived inference. Getter/setter depth remains one.

[GC:] The invocation scope retains receiver/function identities and restores the
previous explicit depth on success, script throw, error and allocation failure.
The existing watcher frame separately retains descriptor, old/requested values
and user data. Unwatch and prototype detachment during callbacks do not remove
those active roots. Recursion suppression and deferred watcher exception policy
are unchanged. There is no public ABI or graph format change.

В загруженных SWF4–8 обработчик Object.watch получает super с глубиной 0
независимо от места хранения функции. Отсутствующий прототип не удаляет фасад;
чтение за концом цепочки возвращает undefined. Сохранены правила preloads и
suppress DefineFunction2, подавление рекурсии watcher и отложенные исключения.
Контекст вызова восстанавливается при успехе и ошибках; активные корни переживают
unwatch, отсоединение прототипов и GC. Незагруженный host сохраняет прежние правила.

Tests: watch_callback_super reproduces the v0.501 failure and covers SWF4–8,
regular functions, DefineFunction2 (SWF7–8), absent/short chains, independent
function storage, numeric/NUL keys and C/interpreter/explicit/automatic JIT routes.
watch_callback_super_roots covers unwatch/detach/GC, captured roots, nested explicit
depth restoration, script throw and error. watch_callback_super_allocation sweeps
new/new[] failures with traced counted data and retry. watch_callback_super_transport
restores pure AVM1 watchers into another loaded context and checks depth/read/GC.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/object/script_object.rs, Watcher::call invokes exec with depth 0.
This source comparison does not certify original Flash Player behavior. Other
callback-depth rules, full SWF compatibility and all roadmap stages remain open.

Validation / Проверка: full Linux x64 C++98 minimal 1017, AGG 1036 and
Cairo 1037 tests passed after execute-permission repairs on 5/7/5 generated
binaries and reruns. Eight targeted ASan/UBSan tests passed after one equivalent
permission repair. Strict C89 depth-header probe and 26 Unicorn branch/error
cases across x86, ARM, ARM64, RV32 and RV64 passed; native x64 ran in host suites.
No foreign operating-system acceptance is claimed.
