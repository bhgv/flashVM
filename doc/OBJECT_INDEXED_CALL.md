# Indexed member calls / Вызов числовых членов — v0.499

The C member-call bridge now recognizes canonical indexed storage on Object/Array
owners before native-registration or __resolve fallback. Indexed functions bind
the original receiver and retain their defining owner, matching AVM1 member calls.
Own/inherited slots, version visibility and nested super follow the same lookup
order as reads. Noncallable indexed data stops the lookup and suppresses __resolve;
C calls return UNSUPPORTED. Loaded AVM1 calls retain undefined for noncallables,
and unloaded AVM1 retains its prior UNSUPPORTED path. The direct execution helper
continues to return undefined for noncallables in either context.

В C API числовой член Object/Array теперь находится в индексном хранилище.
Функция вызывается с исходным this и правильным владельцем для вложенного super.
Скрытые элементы пропускаются по версии SWF. Некликабельный элемент блокирует
дальнейший поиск и __resolve. Контракты ошибок C API и AVM1 сохранены.

Canonical key parsing is unchanged: signed/zero-padded/NUL names remain literal
named properties, and 4294967295 remains outside indexed storage. Receiver and
arguments are snapshotted before clearing an aliased output. Existing call scopes
retain the selected function, owner and capture through detachment, collection,
exceptions and errors. No public C ABI or bytecode format changes.

`object_indexed_call` reproduces v0.498 failure and compares C calls, the direct
execution helper, interpreter and explicit/automatic native routes for host/SWF4–8,
Object/Array owners, own/inherited slots, hidden/noncallable data, high/reserved
keys, aliases, NUL names, nested super and resolver suppression.
`object_indexed_roots` exercises detached/deleted owner/function storage, capture
identity, GC, exceptions and errors. `object_indexed_allocation` sweeps new/new[]
failures, retries, traced counted strings and argument/output aliasing.
`object_indexed_transport` verifies indexed AVM1 methods and nested super after
restoration in another loaded context.

The behavior follows FlashVM's existing member-call contract and the unified
property/defining-depth lookup in Ruffle commit
`2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object.rs` (`search_prototype`) and
`core/src/avm1/object/super_object.rs` (`call_method`). This is source comparison,
not an original Flash Player oracle. Direct function-call/apply super semantics
and the remaining AVM1 model are separate open work. All roadmap stages remain open.

Validation: Linux Release minimal 1005/1005, AGG 1024/1024 and Cairo 1025/1025
pass; minimal passed its first full run, AGG/Cairo required execute-bit repairs
and targeted reruns for 2/4 generated binaries. Six targeted restricted heap/global
ASan/UBSan checks pass: four new member-call tests and existing resolver/member
callback controls. Stack ASan and LeakSanitizer remain disabled. No foreign OS
runtime acceptance or original player oracle is added.
