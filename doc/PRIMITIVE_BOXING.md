# Primitive boxing / Упаковка примитивов — v0.279

C89 `fvm_exec_to_object` accepts a value and returns the same object, a primitive
wrapper, or undefined for null/undefined. Loaded execution resolves the current
global String/Number/Boolean class and uses the common construction service.
Missing or nonconstructable loaded classes yield undefined rather than falling
back to a stale registered constructor. Unloaded hosts keep their constructor
registry policy. Errors preserve output; input/output aliasing is supported.

Loaded GetMember/SetMember converts the name before boxing the receiver. User
name conversion can replace the global class observed by boxing. Primitive
metadata hooks retain priority. Without a hook, operations use a temporary
wrapper, or a fresh bare object when boxing yields undefined. Writes can call an
inherited script setter, then discard the wrapper. Nullish writes complete
normally. Native raw tables keep their separate numeric-key policy.

[GC:] Both original operands, assigned values, converted names and wrappers are
retained through conversion, construction and accessors. The new common helper
uses rooted construction arguments. Slot/count/hook-layout constants have
uppercase names and comments in private `.h` files. The interface accepts values,
with no bytecode, program counter or interpreter-stack dependency.

Общий C-обработчик упаковывает примитив через текущий глобальный конструктор.
SetMember для числа, строки или Boolean теперь может вызвать setter прототипа;
временный объект затем отбрасывается. Запись для null/undefined не прерывает
исполнение. Преобразование имени происходит до выбора конструктора. Хуки
метатаблиц сохраняют приоритет, а нативные таблицы — числовые ключи.

Tests cover C89 host/SWF4–8 wrappers and nullish values, counted NUL strings,
constructor replacement, name-conversion ordering, script setters/getters with
GC, exceptions, metadata hooks, real action dispatch and allocation failures.
The complete primitive boxing/call/With model remains open. In particular, native
constructors that return a scalar still follow the existing constructor API's
error policy; the helper does not certify all coercion or roadmap semantics.

Primary references at pinned Ruffle commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`:
[`value.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs),
`coerce_to_object` / `coerce_to_object_or_bare`, and
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs),
`action_get_member` / `action_set_member`.

Validation: 536 tests pass in each Linux AGG/Cairo profile. Initial launch failures
required execute-bit restoration (4 AGG binaries in the first run, 9 Cairo in
the final run); subsequent checks pass. Six focused heap/global ASan+UBSan checks
pass, plus the final constructor-order callback check; stack/leak coverage is
excluded. All 26 unchanged original SWF traces match exactly in each profile.
