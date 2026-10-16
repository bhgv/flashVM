# Script constructor prototypes / Прототипы скриптовых конструкторов

v0.224: a script function's own constructor prototype in a loaded context
inherits the retained original Object.prototype. Replacing the public
Object.prototype property does not redirect subsequent script prototypes.
The standard-library transaction captures this builtin identity and GC marks
it independently. The function's own prototype property is writable,
nonenumerable and nondeletable (flags 3). Unloaded host function layout remains
unchanged. Function.prototype lookup rules and complete prototype-property
semantics remain open.

В загруженном контексте prototype скриптового конструктора наследует исходный
Object.prototype. Изменение публичного Object.prototype не перенаправляет
новые скриптовые прототипы; исходный объект удерживается отдельным корнем GC.
Собственное свойство prototype функции доступно для записи, не перечисляется
и не удаляется (flags 3). Host-функции сохраняют прежний layout. Полные правила
Function.prototype и prototype-свойств остаются открытыми.

Primary source: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs action_define_function creates the explicit prototype
from the cached builtin object prototype; core/src/avm1/function.rs
FunctionObject::build assigns its attributes.
avm1_function_object_prototype checks SWF5–8 and unloaded host calls, replaces
Object.prototype before collection, then checks inheritance, flags and rejected
delete. It installs a script getter via inherited addProperty, constructs an
instance, collects and transfers the function to another context without standard
installation, then constructs and invokes the getter there. Public ABI and wire
schema are unchanged; existing graph edges and attribute fields carry the change.

Тест проверяет наследуемый addProperty, getter на экземпляре, GC и перенос
конструктора с последующим созданием экземпляра в другом контексте. ABI и схема
передачи прежние. Полная совместимость Player не заявляется.

The unchanged original AS2 v6/v8 probes now finish execution with zero exit and
empty stderr, but their traces still differ from the oracle, including super
prototype checks. They remain failing conformance cases and are excluded from
the passing original corpus. All roadmap stages remain open.

Исходные AS2 v6/v8 завершаются без ошибки, но трассы ещё отличаются от эталона,
включая проверки прототипа super. Эти случаи остаются непроходящими и не входят
в проходящий корпус. Все стадии роудмапа остаются открытыми.

Validation / Проверка: GCC/G++ 13.3 Linux C90/C++98: AGG 383 and Cairo
Release 382 tests passed after execution-permission retries (15 and 1).
Eleven selected AVM1 tests passed in NO_OS_PORTS and heap/global ASan + UBSan;
stack/leak checks remain disabled. All 17 unchanged selected original SWF<=8
probes retained byte-exact traces, zero exits and empty stderr. The separately
measured AS2 v6/v8 exit successfully but remain excluded because traces differ.
No roadmap stage or non-Linux runtime certification is claimed.

Subsequent builtin super projection makes original AS2 v8 byte-exact; see
[AVM1_SUPER_PROTOTYPE.md](AVM1_SUPER_PROTOTYPE.md). После следующей правки
проекции super исходный AS2 v8 совпадает с эталоном.

## v0.465 — loaded script constructors ignore returned values

Loaded AVM1 script construction returns its newly allocated receiver regardless
of the script return value. Only the native factory path propagates its Object
result. The unloaded host namespace retains its previous Object substitution.
This follows FunctionObject::construct in pinned Ruffle
[function.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/function.rs).
function_constructor_return first reproduced replacement of the allocated this.
Host/SWF4–8 test real script bytecode returning undefined/Number/Object/Function/
Array/this, constructor side effects, direct/named C construction, NewObject and
NewMethod in interpreter/attached/automatic JIT with actual native entries and GC.
Two earlier argument-lifetime tests now expect the allocated loaded receiver;
their argument/mutation/GC assertions remain. Full class construction, constructor
identity properties, native callable construction and all stages remain open.

The first full run found two older boxing fixtures relying on script return
substitution. TargetPath now expects undefined for the ignored returned clip and
retains a direct MovieClip positive case; the instanceof callback links the tested
prototype to the actual allocated receiver. GC, exception and callback-count
assertions remain. Final validation v0.465: complete Linux minimal 909/909; AGG,
Linux-hosted NO_OS_PORTS and heap/global ASan+UBSan 8/8 targeted checks each.
Stack ASan/LeakSanitizer disabled; all stages and foreign OS acceptance open.

## v0.466 — script instance constructor identities

Before a loaded script constructor body runs, the new receiver gets own
__constructor__ and, before SWF7, constructor properties pointing to the callee.
Flags are DONT_ENUM only: both are writable/deletable. Forced stored definitions
bypass inherited setters/watchers. A shared private helper stages property/flag
maps before publication and removes exact obsolete own virtual slots afterwards;
OOM preserves the observable receiver's previous metadata. The unloaded host
namespace is unchanged. Reference: FunctionObject::define_constructor_props in
[pinned function.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/function.rs).
function_constructor_identity reproduced missing own metadata and checks host/
SWF4–8, pre-body visibility, inherited accessor traps, flags, hidden enumeration,
write/delete, real GC, serialized graph identity without destination installation,
and actual NewObject entries in interpreter/attached/automatic JIT. Registered
MovieClip construction, native factory identities and all stages remain open.

Validation v0.466: complete Linux minimal 910/910; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 6/6 targeted checks each. Stack ASan/LeakSanitizer
disabled; all stages and foreign OS acceptance remain open.

## v0.467 — registered MovieClip constructor identities

Attach, duplicate and authored sprite placement now define constructor identities
before invoking a registered class, after initObject processing. The explicit
symbol-movie version selects the pre-SWF7 own constructor property, independently
of ambient SWF and destination context; old libraries keep ambient fallback.
SWF7+ initObject constructor data remains untouched, while __constructor__ and
legacy constructor are forced hidden identities. Constructor exceptions retain
the published clip and metadata. Unregistered/default clip construction is not
changed. Reference: post_instantiation / construct_on_existing in pinned Ruffle
[display_object/movie_clip.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object/movie_clip.rs).
movieclip_constructor_identity reproduced missing pre-body metadata. It checks
source SWF5–8 vs ambient6/8, both registration environments, init collisions,
attach/duplicate/authored placement, callback exceptions, GC and graph transfer
without destination standard installation. constructor_identity_allocation sweeps
new/new[] failures through first success for SWF4–8 data/accessor/native-method
slots, preserving old values/flags/cache identity/captures on OOM, retry, forced
replacement, no callbacks, counted strings and release of obsolete cyclic roots.
This is not full display lifecycle or all-allocator/concurrent acceptance.

Validation v0.467: complete Linux minimal 912/912 and AGG 931/931; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 6/6 selected checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.

## v0.468 — stored script constructor prototypes

Loaded Function construction reads own stored prototype data, skipping accessors,
inherited fields and __resolve. The shared class-prototype reader now includes
retained script-virtual data, so instanceof/Implements use the same raw value.
A C native accessor without retained data yields undefined. Host construction
keeps ordinary prototype lookup. Explicit fn.prototype still executes getters.
Reference: ScriptObject::prototype/get_data in pinned Ruffle
[script_object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
function_constructor_stored_prototype reproduced the wrong constructed prototype.
It checks host/SWF4–8, retained Object/primitive virtual data, throwing getters,
native accessors, inheritance, legacy spelling, resolver exclusion, GC, instanceof
and actual NewObject entries in three modes. The old NewObject fixture now checks
that explicit reads invoke its getter while construction does not; all argument,
GC and exception assertions remain. This supersedes earlier assumptions about
loaded prototype-getter execution, while their argument snapshots remain needed.
Native-method-backed prototype fields, registered clip prototype selection and
full class compatibility remain open.

Validation v0.468: complete Linux minimal 913/913; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 8/8 targeted checks each. Stack ASan/LeakSanitizer
disabled; all stages and foreign OS acceptance remain open.

## v0.469 — registered clip stored prototypes use source-movie policy

Known-SWF registered clip construction reads the class's own stored prototype
value, bypassing accessors/inherited fields/resolvers. Retained virtual Object
data remains the live prototype; absent/primitive data yields no Object prototype
instead of rejecting the attachment. The raw field selector accepts an explicit
source-movie version, so SWF6 alternate spelling works under an ambient SWF8 and
SWF8 remains exact under SWF6. Unknown-version unloaded host libraries retain
ordinary lookup. Reference: post_instantiation's constructor.prototype call in
pinned display_object/movie_clip.rs and ScriptObject::prototype/get_data.
movieclip_constructor_stored_prototype reproduced the incorrect receiver chain.
Source6/8 × ambient6/8 cover retained virtual Object/Number, native accessor,
inherited/mixed/missing fields, direct primitive data and throwing getter/resolver
traps across attach/duplicate/authored placement, callback counts, GC and clean
state. Transfer to an opposite-version context without standard installation
checks the new clip against the independently cloned existing clip's prototype;
the custom C native accessor case is excluded from graph transfer. The original
script attachment is dropped, as required by the existing graph contract.
This does not certify primitive __proto__ reflection, native-method-backed
prototype fields, all class lifecycle behavior or any foreign OS.

Validation v0.469: complete Linux minimal 914/914 and AGG 933/933; Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 6/6 selected checks each. Stack
ASan/LeakSanitizer disabled; all stages and foreign OS acceptance remain open.
