# Имена свойств с явной длиной — v0.81 / Counted property names

## Русский

Добавлены C89 API [`fvm_object_get_bytes`](../include/flashvm/value.h#L111), [`fvm_object_set_bytes`](../include/flashvm/value.h#L115),
[`fvm_object_has_own_bytes`](../include/flashvm/value.h#L120), counted варианты property flags/delete и
[`fvm_value_get_property_bytes`](../include/flashvm/metamethod.h#L69)/[`fvm_value_set_property_bytes`](../include/flashvm/metamethod.h#L74). Имена — counted
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
графы; ABI структуры [FVM_Value](../include/flashvm/value.h#L26) не меняется, новые функции добавляются к ABI 3.

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

Добавлены counted [`fvm_object_call_bytes`](../include/flashvm/value.h#L189),
[`fvm_object_define_method_rooted_bytes`](../include/flashvm/value.h#L194),
[`fvm_object_define_accessor_rooted_bytes`](../include/flashvm/value.h#L199) и [`fvm_table_named_at_bytes`](../include/flashvm/table.h#L51).
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

## v0.83 — Глобальные и локальные имена / Global and lexical names

Добавлены counted [`fvm_get_global_bytes`](../include/flashvm/value.h#L133), [`fvm_set_global_bytes`](../include/flashvm/value.h#L137),
[`fvm_register_function_rooted_bytes`](../include/flashvm/value.h#L141) и [`fvm_call_bytes`](../include/flashvm/value.h#L148). Регистрация нативного
имени и capture выполняется одной операцией; legacy registration/call также
используют canonical WTF-8 key. Явный C API call сохраняет native-first правило;
script lookup продолжает учитывать собственное undefined свойство. Нативный
callback копируется перед вызовом, результат сохраняется до снятия native
barrier, bad_alloc и другие C++ исключения переводятся в [FVM_Result](../include/flashvm/flashvm.h#L42).

AVM1 DefineLocal/DeclareLocal/GetVariable/SetVariable/CallFunction и строковое
Enumerate сохраняют полное имя и отличают NUL suffix от legacy prefix. NewObject
при отсутствии полного имени с NUL возвращает UNSUPPORTED, а не вызывает
конструктор по обрезанному префиксу; существующий Function-конструктор с таким
полным ключом работает через обычную lexical/global resolution.

Legacy graphs 1–7 по-прежнему запрещают NUL names, но теперь нормализуют valid
scalar/surrogate-pair keys при restore, поэтому такие ключи доступны и повторно
сериализуются в graph 8. Некорректный WTF-8 и canonical duplicates отклоняются.

Counted global registration/read/write/call now preserve NUL and canonical
UTF-16 identity. Native registration publishes its capture with the entry.
Explicit C native calls retain precedence; script lookup honors own undefined
shadowing. Native results remain guarded and all callback exceptions return
status codes. AVM1 local declarations, variable lookup/assignment, function calls
and string enumeration preserve full names. A missing NUL-suffixed constructor
cannot fall back to its terminated prefix; existing full-key Function values can
construct. Legacy graph keys normalize on restore without admitting old NUL keys.

Remaining boundaries: counted native constructor/stack-function registration,
C-string-only AVM1 named-call/debug helpers, complete duplicate-parameter alias
semantics, Array limits and exact historical/versioned conversions remain open.
This checkpoint does not close roadmap stage 1 or any later stage.

Verification (Linux GCC/G++ 13.3, strict C90/C++98): default AGG 246/246;
static Cairo Release 246/246; examples disabled in both. Headless ASan/UBSan
5/5 focused global/Function2/instruction-budget/native/property regressions;
portable 2/2 new variable/property regressions, live TCP excluded. Stack ASan
and LeakSanitizer remain disabled. New avm1_counted_variables covers lexical
cell independence, undefined declarations/shadowing, rooted globals, native
calls, surrogate aliases, missing-prefix rejection and native exception recovery.
property_counted adds an actual modified legacy graph with surrogate-pair names
and verifies successful restore and graph-8 reserialization.

## v0.84 — Constructor and stack names / Конструкторы и стековые имена

Counted native constructor registration and construction now use canonical keys,
including NUL and lone surrogates. AVM1 NewObject can resolve a full-key native
constructor after lexical/global lookup; a missing name never falls back to a
prefix. Reserved Object remains reserved only for its exact complete name.

Rooted stack registration/call and Lua-style CFunction registration now use the
same canonical name registry. Captures publish with entries in one operation;
lookup allocation failures return OOM before callback entry. Counted auxiliary
field/global helpers preserve stack consumption rules; pushstring/checkstring
variants expose the complete string byte count. Legacy helpers use this bridge.
No value layout or graph version change (ABI 3, graph 8).

Добавлены counted constructor/stack/CFunction API и вспомогательные операции
field/global/string. Имена NUL не обрезаются; пары суррогатов канонизируются.
Native constructor и stack capture устанавливаются атомарно с записью. Служебное
имя Object защищено по полному ключу. AVM1 NewObject вызывает полный native key
без подмены отсутствующего имени префиксом. Инъекции/приёмка остальных стадий
остаются обязательными; данный пункт не закрывает стадию 1.

Regression native_counted_construction covers full-key native construction,
wrong-context capture rejection, reserved names, scalar/pair/lone-surrogate
names, invalid encoding, GC capture retention, FVM and Lua-style stack callbacks,
full-key global-call fallback, AVM1 NewObject and counted field/global/string
helpers. Linux AGG 247/247; Cairo Release 247/247; focused headless ASan/UBSan
4/4 with stack/leak exclusions; portable new test 1/1. Examples disabled in full
profiles. Formal parameter alias identity and remaining conformance gates stay open.

## v0.85 — Parameter aliases and named helpers

UTF-16-equivalent duplicate formal names now select only the rightmost named
arguments alias. Counted AVM1 factory/call, complete debug name queries and
registered-class lookup preserve full identity. See
[FUNCTION_PARAMETER_KEYS.md](FUNCTION_PARAMETER_KEYS.md) for RU/EN behavior,
reproduced failure, test evidence and the remaining Function2/version gates.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call_bytes` | function | [include/flashvm/value.h:148](../include/flashvm/value.h#L148) |
| `fvm_get_global_bytes` | function | [include/flashvm/value.h:133](../include/flashvm/value.h#L133) |
| `fvm_object_call_bytes` | function | [include/flashvm/value.h:189](../include/flashvm/value.h#L189) |
| `fvm_object_define_accessor_rooted_bytes` | function | [include/flashvm/value.h:199](../include/flashvm/value.h#L199) |
| `fvm_object_define_method_rooted_bytes` | function | [include/flashvm/value.h:194](../include/flashvm/value.h#L194) |
| `fvm_object_get_bytes` | function | [include/flashvm/value.h:111](../include/flashvm/value.h#L111) |
| `fvm_object_has_own_bytes` | function | [include/flashvm/value.h:120](../include/flashvm/value.h#L120) |
| `fvm_object_set_bytes` | function | [include/flashvm/value.h:115](../include/flashvm/value.h#L115) |
| `fvm_register_function_rooted_bytes` | function | [include/flashvm/value.h:141](../include/flashvm/value.h#L141) |
| `FVM_Result` | enum | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_set_global_bytes` | function | [include/flashvm/value.h:137](../include/flashvm/value.h#L137) |
| `fvm_table_named_at_bytes` | function | [include/flashvm/table.h:51](../include/flashvm/table.h#L51) |
| `FVM_Value` | struct | [include/flashvm/value.h:26](../include/flashvm/value.h#L26) |
| `fvm_value_get_property_bytes` | function | [include/flashvm/metamethod.h:69](../include/flashvm/metamethod.h#L69) |
| `fvm_value_set_property_bytes` | function | [include/flashvm/metamethod.h:74](../include/flashvm/metamethod.h#L74) |
<!-- source-links:end -->
