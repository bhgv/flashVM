# Indexed ancestor lookup / Числовые свойства предков — v0.498

Selected ancestor reads now include canonical indexed storage on Object/Array
owners. After version visibility checks, indexed data takes the same precedence
as ordinary table reads, before named data/accessor/method fallback. Super reads
start at the selected parent, so receiver-owned values do not shadow parent
slots. The name must be fully terminated and canonical: embedded NUL and signed
or zero-padded aliases remain literal named properties; the reserved 4294967295
name is not converted to indexed storage. No numeric coercion rules change.

Super method-owner selection recognizes those same indexed fields. Calling a
function stored under a numeric key binds the original receiver and records its
actual defining owner; a nested super call therefore starts at the correct next
ancestor. A nearer noncallable indexed value stops lookup and suppresses __resolve.
C member calls retain UNSUPPORTED for noncallables; AVM1 calls return undefined.
Hidden indexed owners are skipped in loaded contexts. Unloaded selected reads
also gain indexed lookup, while unloaded numeric super-write rejection remains.

В чтении выбранной родительской цепочки учитываются числовые элементы Object и
Array. Собственные элементы получателя не заслоняют чтение через super. Скрытые
по версии элементы пропускаются; буквальные ключи с NUL, знаком или ведущим
нулём не преобразуются в канонические индексы. При вызове числового метода
сохраняются исходный this и правильный прототип-владелец для вложенного super.
Некликабельный элемент останавливает поиск и не вызывает __resolve.

The existing call scope now roots the correct indexed owner/function and its
capture. Tests detach the receiver prototype and delete the method during the
callback, then collect and return success, a thrown value or an error. Graph
transport preserves indexed data, pure AVM1 methods and nested defining-owner
lookup. Host behavior changes only where selected ancestor indexed reads/calls
were previously missing; loaded facade write behavior is documented separately
in [SUPER_PROPERTY_WRITES.md](SUPER_PROPERTY_WRITES.md).

Tests: `super_indexed_read` covers host and SWF4–8, Object/Array owners, receiver
shadow data, version visibility, high/reserved numeric names, literal aliases,
NUL names and traced strings through C API/interpreter/explicit/automatic JIT.
`super_indexed_call` covers nested owner traversal, hidden owners, noncallable
shadowing and __resolve suppression. Both reproduce the v0.497 defect.
`super_indexed_roots` covers owner/receiver/capture identity, detachment, GC,
exceptions and errors in all four routes. `super_indexed_transport` covers
restoration in a second loaded context and collection before read/nested call.

Reference: Ruffle `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object.rs` (`get_opt`, `search_prototype`) and
`core/src/avm1/object/script_object.rs` (`get_local_stored`). FlashVM's split
indexed/named storage is an implementation detail of the same property lookup.
No original Flash Player oracle or foreign-OS runtime run is claimed. The full
AVM1/super model, prototype recursion fidelity and all roadmap stages remain open.

Validation: Linux Release minimal 1001/1001, AGG 1020/1020 and Cairo 1021/1021
have passing full-suite evidence after execute-bit repairs and targeted reruns
(5 minimal, 6 AGG, 10 Cairo binaries). Six targeted restricted heap/global
ASan/UBSan tests pass: four new indexed lookup/call/root/transport tests plus the
loaded facade write/watcher controls. One sanitizer binary also required its
execute bit restored. Stack ASan and LeakSanitizer remain disabled. Native x64
interpreter/JIT equivalence is covered; no foreign-runtime acceptance is inferred.
