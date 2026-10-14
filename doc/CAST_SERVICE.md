# Value-based casts / Общий обработчик приведений — v0.282

C89 `fvm_exec_cast` returns the original matching value or null. In loaded SWFs,
primitive inputs invoke the common object-conversion service for its constructor
side effects before returning null. A wrapper never replaces the original input
in the membership test. Null/undefined conversion does not call a constructor.
Constructor exceptions propagate before output changes. Original operands and the
discarded wrapper use named GC root slots; output can alias either input.

The interpreter now only adapts the class/value stack operands and publishes the
service result. Public and private interfaces have no program counter, bytecode
or interpreter-stack parameters. Host cast membership keeps its previous policy.
Object class/interface membership still uses the existing implementation; arbitrary
nonclass constructors/prototype coercions and full InstanceOf behavior remain open.

В загруженном SWF приведение примитива запускает упаковку ради побочных эффектов,
но возвращает null. Конструктор может выполнить GC или выбросить исключение;
оригинальные операнды удерживаются до завершения. Для совпавшего объекта
возвращается исходное значение. Общий C-интерфейс пригоден для вызова из будущего
сгенерированного кода, без зависимости от формата Action-кодов.

Fixtures cover C89 host/SWF4–8 match/mismatch, input/output aliasing, invalid input,
primitive constructor side effects and exceptions, original class survival during
GC, real CastOp dispatch and allocation boundaries. Existing inheritance/interface
fixtures remain in use. All nine roadmap stages stay open.

Primary reference: pinned Ruffle
[`activation.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs)
`action_cast_op`, plus
[`value.rs`](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs)
`instance_of`, which tests the original value after the discarded coercion.

Validation: both Linux AGG/Cairo profiles pass 546 tests after execute-bit repair
and reruns for 6/8 BAD_COMMAND binaries respectively. Four affected heap/global
ASan+UBSan checks pass (no stack/leak coverage). All 26 unchanged original SWFs
match exactly in each profile. This does not complete general InstanceOf semantics.
