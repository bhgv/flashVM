# Resolver callback super / Super обработчика __resolve — v0.504

Loaded SWF4–8 __resolve callbacks now execute with explicit superclass depth zero,
independent of resolver ownership or receiver prototype availability. The facade
exists even for exhausted chains. DefineFunction2 preload/suppress rules remain
in force. Resolver discovery, raw stored-data lookup, primitive skipping,
noncallable Object handling, callback recursion limits and C/AVM1 error contracts
are unchanged. Resolver-returned method invocation is a separate path.

[GC:] The callback scope retains the active Function and restores prior invocation
identities/depth. Existing lookup and receiver scopes keep the original graph;
function capture roots survive removal of __resolve and prototype detachment.
No public ABI or graph wire format changes.

В загруженных SWF4–8 обработчик __resolve теперь получает super с глубиной 0,
независимо от владельца свойства и наличия прототипа получателя. Сохранены
правила DefineFunction2, поиск resolver без вызова getter, предел рекурсии и
контракты ошибок. Корни активного вызова переживают удаление __resolve,
отсоединение прототипов и GC; прежний контекст восстанавливается при ошибках.
Вызов метода, возвращённого resolver, рассматривается отдельно.

Tests: resolve_callback_super reproduces the v0.503 failure and checks SWF4–8,
regular functions, DefineFunction2 (SWF7–8), own/inherited resolver storage,
absent/short prototype chains, counted-NUL missing names and C/interpreter/
explicit/automatic JIT entry routes. resolve_callback_super_roots removes the
resolver and detaches the prototype before GC, checks capture roots, explicit
outer-depth restoration, throws and errors. resolve_callback_super_allocation
sweeps new/new[] failures and retries. resolve_callback_super_transport checks
pure AVM1 resolvers after graph transfer and collection in another loaded context.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
object.rs search_prototype calls resolver.call; function.rs Executable::call
invokes exec with depth 0. This comparison is not an original Flash Player oracle.
Other callback/method-depth rules, full SWF compatibility and all roadmap stages
remain open.

Validation / Проверка: Linux x64 C++98 minimal 1022, AGG 1041 and Cairo 1042
tests passed. Execute permissions were restored on 15/31/34 generated binaries;
minimal was rerun in full because its initial log lacked a final summary, while
AGG/Cairo failed launches were rerun. Five targeted ASan/UBSan tests and a strict
C89 depth-header syntax probe passed. No foreign-OS acceptance is claimed.
