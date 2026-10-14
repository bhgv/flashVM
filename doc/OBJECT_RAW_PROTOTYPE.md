# Raw prototype values / Raw-значения прототипа

v0.227 adds a tested subset of loaded-SWF `__proto__` data semantics. Assigning
a Number, Boolean, counted String, null or undefined retains that exact data
value and clears the object-only inheritance edge, without coercing the input.
Assigning an object publishes both the raw slot and the edge. Host/unloaded
prototype accessor policy remains unchanged. The shared GetMember/SetMember
C service uses this property bridge too; no interpreter-only implementation.

В загруженных SWF присваивание примитива в `__proto__` сохраняет значение,
включая NUL в строке, и прекращает наследование. null отличается от undefined
при чтении. Присваивание объекта восстанавливает цепочку. Нативный host-режим
сохраняет прежнюю политику; это не утверждение полной совместимости прототипов.

[GC:] Preparation copies the property's maps and stages default flags before
atomically publishing data/attributes/edge. Failed allocation leaves all three
unchanged. The copied value comes from the ordinary owned-string property bridge.
Raw primitives are normal traced properties and already travel through graph
schema 8. An existing raw slot follows explicit host relinking; successful
ordinary deletion removes its edge. Cycle/cross-context/depth rejection remains
FlashVM's bounded host policy; reference cyclic-prototype behavior is still open.

[GC:] Данные, атрибуты и ссылка публикуются после успешной подготовки. Ошибка
выделения памяти не разрывает старую цепочку и не оставляет частично созданный
слот. GC и канал используют обычные свойства; схема сериализации не менялась.
Изменённые пользователем флаги сохраняются. Циклы и чужие контексты запрещены
по ограниченной политике FlashVM, а не заявлены как точная модель Flash Player.

Primary implementation reference: Ruffle commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object/script_object.rs`: new_impl defines ordinary raw data;
proto reads stored data without getter invocation and only Object values are
followed by inheritance lookup. This checkpoint implements primitive retention,
not the complete raw-slot initialization/accessor/property-case model.

Tests: object_raw_prototype.cpp covers SWF4–8 and unloaded host behavior,
primitive/null/undefined/string data, flags, GC, graph transfer, relinking,
cycle rejection, deletion and direct C execution-service calls.
object_raw_prototype_allocation.cpp injects allocation failures until first
success and verifies root/native-depth/data/attribute/edge restoration.
Full Linux AGG/default profile: 428 tests/examples passed. Four selected tests
passed in the minimal heap/global ASan + UBSan profile; stack/leak coverage is
disabled as documented in AVM1_DECOMPOSITION.md.

The original unchanged SWF6 object_prototypes fixture from that pinned commit
now matches its raw primitive value section, including number/boolean/string/
null/undefined and typeof results. Its complete oracle still differs on builtin
own-slot membership, casing and deletion scenarios; it is not a passing corpus
entry. SWF8 biturshift_swf8 remains byte-exact. Baseline v0.226 comparison keeps
five other selected outputs/exits unchanged. Two selected probes are SWF15/17,
outside the declared SWF<=8 scope; The actions-only AS2/preload
comparison needed a different host policy; subsequent root-timeline/quit-ack
validation passes both unchanged originals (see EXECUTION_CHECKPOINT.md). These observations
supersede assuming the prior session's corpus was present or revalidated here.

Исходный object_prototypes ещё не проходит целиком: raw-примитивы исправлены,
но собственность встроенного слота, регистр имён и некоторые удаления расходятся
с эталоном. Повторно проверенная biturshift_swf8 совпадает полностью. Остальные
наблюдаемые отказы/расхождения уже есть в v0.226. Все стадии роудмапа открыты.
