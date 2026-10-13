# Имена свойств с явной длиной — v0.81 / Counted property names

## Русский

Добавлены C89 API `fvm_object_get_bytes`, `fvm_object_set_bytes`,
`fvm_object_has_own_bytes`, counted варианты property flags/delete и
`fvm_value_get_property_bytes`/`fvm_value_set_property_bytes`. Имена — counted
UTF-8/WTF-8 без требования терминатора. NUL и одиночные суррогаты сохраняются;
пара UTF-16 суррогатов и её scalar UTF-8 форма обозначают одно свойство.
Некорректные последовательности отклоняются до изменения объекта.
Старые get/set/has-own/flags/delete API используют тот же путь с strlen.

Внутренний путь чтения/записи теперь использует owned counted key: prototype
lookup, getters/setters, watch, `__index`/`__newindex`, super property lookup и
перечисление сохраняют имя полностью. Методы Object hasOwnProperty,
isPropertyEnumerable, addProperty, watch/unwatch и ASSetPropFlags учитывают
длину. AVM1 InitObject/GetMember/SetMember передают counted имя. Числовой bridge
Array/Object включается только для ключей без внутреннего NUL.

Формат графа 8 отделяет counted имена свойств от terminated идентификаторов
host callbacks. Читатель принимает версии 1–8; старые версии не разрешают NUL
в именах. В версии 8 reader проверяет WTF-8, нормализует эквивалентные ключи и
отклоняет дубликаты после нормализации; writer не выпускает некорректные ключи.
Очередь, файл и TCP поддерживают версию 8. Старые readers не читают новые
графы; ABI структуры FVM_Value не меняется, новые функции добавляются к ABI 3.

Границы: counted native method/accessor registration, method/global calls,
AVM1 variable/function names и length-aware native debug/table-name queries
ещё требуют отдельного прохода. Полная SWF-version semantics и историческая
Flash-сверка не подтверждены; стадии 1–9 остаются открытыми.

## English

New counted C89 object get/set/has-own, attribute/deletion and primitive-property
APIs preserve embedded NUL and lone surrogates. UTF-16-equivalent WTF-8 surrogate
pairs and scalar UTF-8 forms address one canonical property. Invalid encoding
is rejected before mutation. Terminated property APIs use the counted bridge.

Prototype/accessor/meta/watch/super property lookup and enumeration preserve
all bytes. Object property predicates, addProperty, watch/unwatch and
ASSetPropFlags use counted names; AVM1 InitObject/GetMember/SetMember do too.
Numeric Array/Object aliases require the entire key, excluding embedded NUL.

Graph 8 permits counted property names while keeping host callback identifiers
terminated. Readers accept graphs 1–8; old versions still reject NUL names.
Version 8 validates WTF-8 and rejects duplicate canonical names. Writers reject
invalid/noncanonical names. Queue/file/TCP framing accepts graph 8; older readers
cannot read new graphs. Public value layout stays ABI 3; additions are functions.

Counted native registration/calls, AVM1 variable/function names and native debug/
table name-size queries remain separate work. Historical version conformance
and full roadmap acceptance remain open.

## Проверки / Verification

Linux x86_64, strict C90/C++98: default static AGG/fonts/codecs/SQLite,
examples disabled, 244/244 tests. New property_counted covers NUL/pair/lone
surrogate keys, prototype lookup, attributes/deletion, enumeration, watch,
AVM1 GetMember, queue/file/TCP, GC and malformed/duplicate graph rejection.
property_counted_allocation injects every allocation failure until successful
get/set and graph restore; failed writes retain the previous value and failed
restores retain object counts. Portable NO_OS_PORTS: 2/2 new tests, TCP excluded.
Headless ASan/UBSan: 5/5 new/property/serialization/string/generic-array checks.
ASan stack instrumentation and LeakSanitizer are disabled: exception unwinding
and task enumeration cannot read sandbox /proc. Heap/global ASan and UBSan
remain enabled. No other-platform execution or full stack/leak proof is claimed.

## v0.82 — Нативные методы / Native members

Добавлены counted `fvm_object_call_bytes`,
`fvm_object_define_method_rooted_bytes`,
`fvm_object_define_accessor_rooted_bytes` и `fvm_table_named_at_bytes`.
Старые terminated registration/call API также канонизируют WTF-8. Чтение
нативного метода возвращает стабильный cached Function для полного ключа.
Prototype dispatch, accessor receiver и traced capture сохраняют прежние
правила. AVM1 CallMethod/NewMethod используют полный нормализованный ключ.
Новые функции не меняют ABI layout или graph 8. Terminated глобальные
функции/конструкторы и AVM1 variable names ещё требуют отдельного прохода.

Counted native method/accessor registration, member calls and table name-size
queries now preserve complete keys. Legacy registrations/calls normalize the
same WTF-8 representation. Cached Function identity, inherited receiver binding,
rooted captures and graph transport remain intact. AVM1 CallMethod/NewMethod
use full normalized names. Global registration/calls and variable-name handling
remain open; full stage-1 acceptance is not claimed.

Verification: Linux default AGG 245/245, examples disabled. New
native_counted_members covers embedded-NUL member names, scalar/pair/lone
surrogates, invalid names, inherited dispatch, shadowing, setters, stable method
Function identity, GC capture retention, AVM1 CallMethod, graph restore and queue
transfer. Headless ASan/UBSan targeted 4/4; portable new test 1/1. Stack/leak
sanitizer exclusions remain as above. No other-platform runtime certification.
