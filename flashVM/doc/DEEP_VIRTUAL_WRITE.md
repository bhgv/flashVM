# Deep virtual write dispatch / Поиск дальних setter — v0.495

Loaded SWF4–8 ordinary property assignment searches past inherited data and
native-method entries for a visible virtual property farther up the prototype
chain. Own data, including indexed slots and version-hidden native methods,
prevents that search. Own script virtuals retain existing setter behavior.
Inherited readonly data does not stop the search. Version-hidden inherited
virtuals are skipped; a visible getter-only virtual consumes the assignment.

Named writes, canonical table-index writes and native Array pre-growth now use
the same own-data membership rule. An inherited virtual consumes the write before
Array length growth, including signed/noncanonical decimal names. An own virtual
runs after growth. Intermediate data and prototype descriptors are retained;
setters bind the original receiver. Ordinary read lookup still returns the nearer
data property. Host-only lookup and explicit selected-super writes retain their
prior policy; this change does not certify those remaining super-write semantics.

В SWF4–8 запись ищет virtual setter за обычными унаследованными свойствами.
Собственное data/method свойство блокирует поиск, даже если оно скрыто по версии.
Наследуемый readonly не блокирует дальний setter. Видимый virtual без setter
поглощает запись. Дальний setter получает исходный this; промежуточные данные
сохраняются. Array.length не растёт при наследуемом virtual. Правила чтения
не меняются: чтение возвращает ближайшее обычное свойство.

Deferred watcher exceptions keep existing inherited-virtual policy: successful
inherited setter/getter-only dispatch consumes the watcher exception; an exception
from the setter wins. The watcher exception remains rooted through GC in the
setter. Script descriptor and receiver roots unwind on return/error.

`object_deep_virtual_write` covers all five SWF versions, Object/Array,
named/canonical/signed numeric keys, intermediate data/native methods, own data,
hidden native methods, own virtuals, readonly/hidden data, hidden inherited virtuals,
C API and interpreter/explicit/automatic JIT. It fails against v0.494.
`object_deep_virtual_watch` covers deferred exceptions, setter exceptions, getter-only
virtuals, counted NUL names and collection inside watcher/setter callbacks.
`object_deep_virtual_transport` covers pure AVM1 setter graph serialization and
collection after restoration in another loaded context, including counted NUL names.

Reference: Ruffle `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object.rs` (`set_internal`) and
`core/src/avm1/object/script_object.rs` (`has_own_property`, `has_own_virtual`).
This is source comparison, not a new original Flash Player oracle run. All roadmap
stages remain open, including non-Linux native runtime acceptance.

Validation: Linux Release minimal 993/993 and AGG 1012/1012 after restoring
execute permissions on affected generated binaries. Five targeted restricted
ASan/UBSan tests pass (three new deep-dispatch tests plus existing inherited
Array setter and thrown-watcher tests). No foreign OS runtime gate is closed.

Loaded selected-super writes are addressed separately in [v0.497](SUPER_PROPERTY_WRITES.md).
