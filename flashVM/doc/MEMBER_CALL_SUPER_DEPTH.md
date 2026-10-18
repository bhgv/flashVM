# Named member invocation depth / Глубина super обычного метода — v0.506

Loaded SWF4–8 ordinary named method calls now capture lookup depth before invoking
a getter and apply a minimum superclass depth of one. Own and immediate-prototype
methods therefore use depth one; deeper inherited methods retain their lookup
distance. The actual defining owner remains separately rooted and available to
native/script invocation metadata. Getter deletion or receiver-prototype changes
do not turn the captured depth into undefined super. C and AVM1 member helpers
use the same rule; unloaded host calls retain owner-derived inference. Direct
Function invocation and explicit-super selected-method policy are unchanged.

[GC:] Existing scopes retain the defining owner, Function, receiver and arguments;
the captured numeric depth does not add graph edges. Previous invocation metadata
is restored on success, script throw, error and allocation failure. No public ABI
or graph wire format changes.

В загруженных SWF4–8 глубина super обычного метода сохраняется до вызова getter
и составляет не менее 1. Для собственного метода и метода ближайшего прототипа
это глубина 1, для более дальних предков — расстояние до владельца. Фактический
владелец свойства хранится и удерживается GC отдельно. Удаление getter и изменение
прототипа внутри него не уничтожают super последующего метода. Незагруженный host
сохраняет прежнее поведение; прямой вызов Function не меняется.

Tests: member_call_super_depth reproduces the v0.505 failure and covers SWF4–8,
regular functions, DefineFunction2 (SWF7–8), own/inherited data or getter-returned
methods, absent/short chains, named/numeric/NUL keys and C/interpreter/explicit/
automatic JIT routes. member_call_super_roots deletes descriptors and detaches
prototypes in getters, collects in getter/method, checks separate owner/capture
roots, outer explicit-depth restoration, throws/errors and an unloaded host control.
member_call_super_allocation sweeps new/new[] failures and retries missing-prototype
own calls. member_call_super_transport checks own/inherited pure AVM1 methods after
graph transfer and collection in another loaded context.

Existing indexed-call fixtures now place the parent implementation at the second
ancestor for loaded own methods. Property-specific depth-zero fixtures create an
explicit activation instead of relying on an own named method to infer zero;
their existing property, GC and watcher assertions are retained.
Старые заготовки индексных вызовов и фасадов глубины 0 приведены к новому правилу:
проверки самих свойств, watcher и GC сохранены, а глубина фасада задаётся явно.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
object.rs search_prototype and call_method, which uses depth.max(1) before exec.
This comparison is not an original Flash Player oracle. Explicit-super selected
method mutation, direct function-call depth and other callback rules remain open;
complete SWF compatibility and all roadmap stages remain open.

Validation / Проверка: after updating the two shared fixtures, full Linux x64
C++98 suites were rerun: minimal 1030, AGG 1049 and Cairo 1050 tests passed.
The final minimal run required one generated-binary execute-permission repair
and rerun; final AGG/Cairo suites passed directly. Twelve targeted ASan/UBSan
tests, a strict C89 depth-header probe and 26 Unicorn branch/error cases across
x86, ARM, ARM64, RV32 and RV64 passed. Native x64 ran in host suites; foreign
operating systems and complete SWF compatibility remain unverified.
