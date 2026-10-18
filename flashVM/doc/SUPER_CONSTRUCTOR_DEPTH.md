# Super constructor depth and roots / Глубина и корни конструктора super — v0.508

Loaded SWF4–8 explicit-super constructor calls now execute at facade depth plus
one, independent of __constructor__ storage and availability of the next parent.
A constructor getter cannot remove the callee's super by changing prototypes.
The base object and its originally selected parent are rooted before getter
execution, so disconnected identities survive collection through invocation.
The actual getter still receives the selected base as this; the constructor
receives the original facade receiver. Unloaded host depth inference is unchanged.

[GC:] Separate base/parent roots cover property lookup before the Function call
scope exists. Existing Function/capture/receiver roots and metadata restoration
cover completion, script throw, callback error and allocation failure. No public
ABI or graph format change. Native constructor dispatch/return-policy equivalence
and full constructor semantics are not certified by this depth/root fix.

В загруженных SWF4–8 конструктор через явный super получает глубину исходного
фасада плюс 1, независимо от расположения __constructor__ и наличия следующего
прототипа. База и выбранный родитель удерживаются GC ещё до вызова getter.
Getter получает базу как this, конструктор — исходного получателя фасада.
Удаление свойства, разрыв цепочек и GC внутри getter не уничтожают корни и
сохранённую глубину. Незагруженный host сохраняет прежние правила глубины.

Tests: super_constructor_depth covers SWF4–8, regular functions, DefineFunction2
(SWF7–8), facade bases/short or exhausted chains, own/deeper __constructor__, raw
and getter-returned constructors, original receiver/public ancestor reads and
C/interpreter/explicit/automatic JIT routes. super_constructor_depth_roots
reproduces the pre-fix failure, deletes the descriptor, disconnects receiver/base
prototypes and collects in getter/constructor; verifies base/parent/capture roots,
constructor identity and outer-depth restoration on success/throw/error.
super_constructor_depth_allocation sweeps new/new[] failure/retry with counted
arguments and a missing next parent. super_constructor_depth_transport restores
pure AVM1 constructors and facades, including exhausted chains, into another
loaded context before invocation and GC.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
super_object.rs call reads __constructor__ from base_proto without resolver
fallback and invokes exec_constructor at facade.depth + 1. Source comparison is
not an original Flash Player oracle. Direct-function depth, complete constructor
semantics, full SWF compatibility and all roadmap stages remain open.

Validation / Проверка: full Linux x64 C++98 minimal 1038, AGG 1057 and Cairo
1058 tests passed after execute-permission repairs on 3/6/3 generated binaries
and reruns. Six targeted ASan/UBSan tests passed after one equivalent permission
repair. A strict C89 depth-header probe passed. Foreign-OS acceptance and full
SWF compatibility remain open.
