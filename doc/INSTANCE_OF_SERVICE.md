# InstanceOf service / Общая проверка принадлежности — v0.283

C89 `fvm_exec_instance_of` returns a Boolean through the ordinary value ABI.
Loaded primitives return false without boxing the class operand. For an object,
the class operand is boxed through the current global primitive class when
needed. Only its own stored `prototype` data is read, without invoking an accessor
or inheriting that field. The prototype value is then boxed if necessary. The
prototype field spelling uses the SWF4–6 Flash case policy and exact SWF7–8 names.
The class operand need not itself be callable. Host-only calls retain the strict
existing `fvm_value_instance_of` class API policy.

Ordinary prototype links are walked separately from interface edges. Interface
membership follows declared interface edges, not the ordinary prototype chains
of interface nodes. Existing private transport metadata stores interface class
objects; this service adapts their current own object-valued prototype fields.
That was the v0.283 representation. Since v0.284, loaded declarations retain
frozen prototype edges with an explicit mode; legacy graphs/host declarations
keep constructor references. See [INTERFACE_SNAPSHOTS.md](INTERFACE_SNAPSHOTS.md).
Cycles and excessive graphs retain bounded runtime error policies.

[GC:] Original value/class, boxed class and prototype are fixed roots through
constructor callbacks. Coercions finish before the raw graph walk, which invokes
no getters or user callbacks. Failed coercion/allocation preserves output; input
and output may alias. The interpreter only moves stack operands. CastOp shares
object membership while keeping its deliberate discarded primitive boxing.

Для объекта проверка упаковывает класс и его сохранённое поле prototype, но не
вызывает getter этого поля. Унаследованное поле prototype не подходит; сам класс
может быть обычным объектом. Примитивный проверяемый аргумент сразу даёт false.
Граф интерфейсов отделён от обычных прототипов. Исходные значения удерживаются
при GC и исключениях в конструкторах; интерфейс не зависит от байткода.

Tests cover C89 own/inherited/accessor/class/case behavior, aliasing, class and
prototype boxing with GC and exceptions, primitive short-circuiting, interface
cycles, exclusion of interface prototype ancestry, actual InstanceOf/CastOp and
allocation failures in traversal and both boxing phases. Original host class API
and legacy inheritance fixtures remain in use. Full property case folding,
interface snapshots, removed clips, super and other roadmap boundaries stay open.

Primary references at Ruffle commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`:
[`value.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs)
`instance_of`,
[`script_object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
`prototype`/`get_data`,
[`object.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs)
`is_instance_of`, and
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`action_implements_op`.

Validation: Linux AGG and Cairo each pass all 550 tests after execute-bit repair
and reruns for 12/6 BAD_COMMAND binaries respectively. The expanded host-policy
and three-phase allocation fixtures also pass in both profiles. Eight affected
heap/global ASan+UBSan checks pass with stack/leak coverage excluded. All 26
unchanged original SWFs match in each profile. No roadmap stage is completed.
