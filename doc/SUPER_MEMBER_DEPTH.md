# Explicit-super member depth / Глубина метода через super — v0.507

Loaded SWF4–8 explicit-super member calls now preserve the facade-relative lookup
depth independently of the selected property's owner. The invocation depth is
facade depth + selected lookup distance + one. Getter deletion and prototype
mutation no longer remove the returned method's super. Resolver results retain
the v0.505 base + one rule; unloaded host calls retain owner-derived inference.
The existing private depth bound is checked before adding the parent offset.

[GC:] The actual selected owner remains separately rooted through getter and
method execution. Existing Function/capture/receiver roots and invocation-scope
restoration cover success, throw, callback error and allocation failure. No
public ABI or graph wire format changes.

В загруженных SWF4–8 при вызове метода через super сохраняется глубина исходного
фасада плюс расстояние поиска и один переход к предку. Фактический владелец
свойства удерживается отдельным корнем GC. Удаление getter и отсоединение
прототипа внутри него не уничтожают super возвращённого метода. Сохранены
контракты ошибок и прежнее поведение незагруженного host.

Tests: super_member_depth covers SWF4–8, regular functions, DefineFunction2
(SWF7–8), facade bases 0–2, lookup distances 0–2, stored/getter-returned methods,
named/numeric/NUL keys, public ancestor reads, GC and C/interpreter/explicit/
automatic JIT routes. super_member_depth_roots reproduces the v0.506 failure,
deletes descriptors, detaches prototypes and collects in getter/method, checking
separate owner/capture roots and outer-depth restoration on success/throw/error.
super_member_depth_allocation sweeps new/new[] failure/retry with counted data.
super_member_depth_transport restores pure AVM1 methods and explicit facades into
another loaded context before invoking and collecting.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
super_object.rs call_method uses facade.depth + lookup.depth + 1. Source comparison
is not an original Flash Player oracle. Constructor and direct-function depth,
complete SWF compatibility and all roadmap stages remain open.

Validation / Проверка: full Linux x64 C++98 minimal 1034, AGG 1053 and Cairo
1054 tests passed after execute-permission repairs on 7/4/6 generated binaries
and reruns. Six targeted ASan/UBSan tests and a strict C89 depth-header probe
passed. Foreign operating systems and complete SWF compatibility remain open.
