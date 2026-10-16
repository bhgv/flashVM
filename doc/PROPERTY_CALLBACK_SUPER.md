# Property callback super / Super в getter и setter — v0.501

Loaded SWF4–8 executable addProperty getters/setters run with explicit superclass
depth one, independent of descriptor ownership or function storage. Super is
available even when the receiver lacks a prototype; exhausted parent reads return
undefined. C API, interpreter and native entry routes use the same private helper.
Unloaded host callbacks keep their existing owner-derived behavior. Nonfunction
getter/setter fallback and loaded super-as-callback rejection are unchanged.

The invocation scope retains callback identities and the depth used for super.
The surrounding accessor scope separately retains the actual lookup owner and
descriptor; setter lookup metadata remains unchanged. Consequently own writable
setters still update retained data, readonly setters preserve data while their
script side effects run, and inherited setters do not overwrite ancestor data.
Watch/deferred-exception policy, property flags and Array pre-growth are preserved.
No public ABI or graph wire format changes.

В загруженных SWF4–8 getter/setter, установленные через addProperty, получают
super с глубиной 1 независимо от владельца дескриптора. Контекст вызова функции
и фактический владелец свойства сохраняются раздельно: прежние правила readonly,
записи сохранённых данных и inherited setter не меняются. Отсутствующий прототип
не удаляет фасад super. Незагруженный host сохраняет прежнее поведение.

Tests: `property_callback_super` reproduces the v0.500 failure and covers own/deep
inherited descriptors, short/missing chains, Object/Array receivers, named/numeric/
signed/NUL keys, readonly data, retained-data policy, host control and all four
C/interpreter/explicit/automatic JIT routes. `property_callback_super_roots` deletes
the descriptor and splits the receiver/prototype chain before GC, verifying the
separate descriptor owner/capture roots and depth restoration on success/throw/error.
`property_callback_super_allocation` sweeps new/new[] failures, traced counted
strings, partial callback side effects and retry. `property_callback_super_transport`
restores pure AVM1 virtuals into another loaded context, then reads/writes and collects.

Primary source: Ruffle `2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object/script_object.rs` (`call_getter`/`call_setter` invoke depth 1).
Source comparison is not an original Flash Player oracle. Watcher/comparator and
other callback-depth rules, complete AVM1 and all roadmap stages remain open.

Validation / Проверка v0.501: full Linux x64 C++98 suites passed in minimal
(1013 tests), AGG (1032) and Cairo (1033) builds after restoring execute permission
on 5/6/3 generated test binaries and rerunning those tests. Eight targeted
ASan/UBSan tests passed, including all four new callback tests. The depth headers
also passed a strict C89 syntax probe. These runs do not certify other operating
systems or complete SWF compatibility.

Watcher depth is separately fixed at zero in [v0.502](WATCH_CALLBACK_SUPER.md).
