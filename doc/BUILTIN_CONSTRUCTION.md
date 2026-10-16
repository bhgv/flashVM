# Loaded builtin construction and invocation — v0.473

The loaded SWF4–8 paths for Date, Error, empty/nullish Object and Array construction
now prepare an instance using the callee's own stored prototype data. They install
hidden writable/deletable `__constructor__` and, before SWF7, `constructor` links
before argument conversion or inherited message setters. Native prototype getters,
inherited properties and `__resolve` do not participate in that selection. Stored
script-accessor data and native-method Function wrappers remain supported. A missing
or primitive prototype produces no Object prototype chain. Unloaded host lookup
and conversion contracts remain unchanged.

Date now allocates before its numeric argument-prefix projection. Mutation of the
class's prototype from `valueOf` cannot redirect the pending receiver. Existing
calendar, fractional timestamp, undefined-prefix and host rules are preserved.
Error preserves its message value and publishes constructor metadata before an
inherited message setter. Setter failures do not publish an API result; a receiver
explicitly saved by the setter remains visible, with its already installed metadata.

The following loaded invocations are distinct from construction:

| Invocation | Behavior |
| --- | --- |
| `Object()` / `Object(undefined)` / `Object(null)` | Bare Object, no prototype or own constructor identities. |
| `Array(...)` | Native Array using the context's retained builtin Array prototype, without own constructor identities. |
| `Error(message)` | Initialize the calling receiver's message when supplied and not undefined; return undefined. |
| `Error.prototype.toString()` | Return the message value unchanged, including numbers, nullish values and Objects. |

Array() and Error() have appended stable callback transport IDs; old constructor
IDs retain their meanings. Imported Array functions remain callable without
standard installation, using the existing own-native-method fallback when the
receiving context has no retained builtin Array prototype. This proves callable
transport, not identical prototype identity in such a destination.

Internal Function parameter, constant-pool and Function2 register arrays no longer
consult public `Array.prototype` getters while being assembled. These internal
arrays retain their existing explicit null prototype links. The new Array
prototype matrix exercises this fix while creating actual interpreted/JIT bodies.

## Evidence and limits

- `date_constructor_stored_prototype`, `error_constructor_stored_prototype`,
  `array_constructor_stored_prototype`, `object_constructor_stored_prototype`:
  SWF4–8, ten stored/prototype forms, direct/named C factories, NewObject/NewMethod
  in interpreter, attached native and automatic JIT modes; identities and GC.
- `error_constructor_setter`: success, plain errors and counted Object exceptions;
  pre-setter identity visibility, callback-published receivers and post-return GC.
- `base_constructor_calls`: bare Object and builtin Array function semantics,
  throwing prototype accessors, all call routes and graph transport without
  destination standard installation.
- `error_legacy_values`: six message value kinds, embedded NUL, skipped coercion
  traps, omitted arguments, receiver selection, native/interpreted calls and
  transported callable/constructable Error classes.
- `function_metadata_array_prototype`: host/SWF4–8 legacy/Function2 creation,
  retained parameter/pool/register arrays and actual interpreter/attached/automatic
  native invocation with a throwing public Array prototype getter.
- `native_instance_allocation`: cold prototype wrappers and factory allocation
  boundaries through Number, String, Boolean, Date, Error, Object and Array;
  null failure output, restored scopes, collection and retry after new/new[] OOM.

All nine checks fail against the v0.472 source and pass with these changes.
Native factory barriers still reject in-callback collection; actual collection is
checked after return. These are not all-allocator, concurrent collector or original
Flash Player conformance proofs. Array numeric-length coercion/clamping remains
under its previous restricted policy; primitive Object boxing still uses the
existing global constructor services. Other native factories, full class lifecycle,
foreign CPU/OS runs and every roadmap stage remain open.

Reference: Ruffle commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
[function.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/function.rs),
[date.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/date.rs),
[object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/object.rs),
[array.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs),
and [Error.as](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/Error.as).

## Русский

Для загруженных SWF4–8 Date, Error, пустой Object и Array выбирают собственный
сохранённый prototype и получают скрытые свойства конструктора до преобразований
и setter-ов. Date не меняет выбранную цепь при мутации класса из valueOf. Error
передаёт исходный message, а setter видит метаданные уже в первом вызове.
Ошибка setter-а оставляет выход API пустым; явно опубликованный им объект живёт
по обычным правилам достижимости.

Пустой Object() возвращает объект без прототипа. Array() выбирает сохранённый
встроенный прототип контекста, обходя изменённое свойство класса. Эти вызовы не
добавляют собственные свойства конструктора. Error() записывает message в
получателя вызова и возвращает undefined; его toString возвращает message без
преобразования типа. Новые callback ID дописаны без изменения прежних значений.
Перенос callable-классов проверен без установки библиотеки в приёмнике; для Array
без встроенного прототипа действует прежний fallback собственных native-методов.
Равенство полной цепи прототипов в таком приёмнике не сертифицировано.

Внутренние массивы параметров, констант и регистров Function2 не вызывают getter
Array.prototype. Проверены C API, NewObject/NewMethod/CallFunction/CallMethod в
трёх режимах, исключения, GC после возврата и перебор отказов new/new[] семи фабрик.
Все девять проверок воспроизвели ошибки на v0.472. Старые ограничения числовой
длины Array, boxing Object, native-барьера GC, остальных фабрик и всех стадий
сохраняются. Реальных запусков на других CPU/ОС эта серия не добавляет.

Validation v0.473: complete Linux minimal 926/926 and AGG 945/945; nine
selected heap/global ASan+UBSan checks passed (eight together, then the new
Function2 metadata check). Stack ASan and LeakSanitizer remain disabled.
Three scoped constant headers passed a strict C89 syntax probe. Executable
permissions were repaired before final native runs. A one-record malformed-wire
fixture explicitly strips new constructor edges; normal roundtrip tests keep them.
Foreign CPU/OS acceptance and all nine roadmap stages remain open.

Проверка v0.473: полные Linux minimal 926/926, AGG 945/945; девять выбранных
проверок heap/global ASan+UBSan прошли. Stack ASan/LeakSanitizer отключены.
Три новых заголовка констант прошли C89. Приёмка других CPU/ОС и все стадии открыты.

## v0.474 follow-up / Продолжение

[Current-global primitive boxing](PRIMITIVE_GLOBAL_BOXING.md) replaces the stale
registry/capture selection for loaded Object(value) and call/apply. The previous
v0.473 boxing limitation above is historical; full primitive/native compatibility
and other listed limitations remain open.

Выбор актуального глобала для Object(value)/call/apply исправлен в v0.474;
прежнее ограничение v0.473 выше относится к исторической точке.
