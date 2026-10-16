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
