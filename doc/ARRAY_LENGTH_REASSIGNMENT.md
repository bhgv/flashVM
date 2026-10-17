# Recreated and virtual Array.length / Пересозданная и виртуальная длина — v0.528

Exact `length` writes on a loaded native Array now perform AVM1 conversion and
Number-tail truncation even after the original native accessor has been deleted
or replaced. This happens after watch processing but before readonly checks,
copy/publication and virtual setter selection. The stored incoming value remains
typed; truncation reads own raw data after conversion callbacks, ignores length
getters/prototypes, and does not coerce a nonnumeric old value. DONT_DELETE tails
survive and every deletion charges the shared work quota.

Loaded `addProperty` now permits Array length. Replacing the native accessor
captures its original typed data and attributes without calling its getter. The
new script descriptor owns that data; obsolete native cache edges are cleared.
The embedding-only policy is preserved. Virtual getters are not invoked for
truncation, and script setters retain their existing readonly/stored-data rules.

The callback may delete the own length, replace ordinary data, or install the
native accessor. Subsequent dispatch uses the live descriptor. If a native
accessor was reinstalled, publication uses the conversion already performed;
valueOf is not called a second time. Error/throw propagation preserves callback
side effects, leaves incoming data unpublished, and unwinds roots/setter state.
[GC:] The ordinary write's existing value-vector and receiver roots retain a
coercion input even after it deletes all global edges and collects.

В загруженном native Array точное присваивание `length` преобразует значение и
удаляет хвост старого Number даже после удаления или замены штатного accessor.
Подготовка идёт после watch и до readonly/выбора setter; входное значение
сохраняет исходный тип. Читаются собственные raw-данные после callback, getter
и прототип не вызываются. Старое нечисловое значение повторно не преобразуется.
`addProperty` теперь позволяет виртуальную length у загруженного Array и
сохраняет прежние данные/флаги; неактуальная native-ссылка освобождается.
После callback выбирается живой дескриптор, повторного valueOf при установке
native-accessor нет. Ошибки не откатывают уже выполненные побочные эффекты.

Tests / проверки: `array_length_reassignment` covers absent, numeric, String,
virtual and original-native-to-virtual descriptors, readonly/DONT_DELETE,
SWF4–8, C API/interpreter/explicit/automatic JIT and graph roundtrips.
The roots variant collects detached graphs during descriptor deletion,
replacement and native reinstatement, including errors/throws. The allocation
variant sweeps every assignment boundary and verifies GC/retry/unwinding.
The ordinary numeric-tail regression fails against the pre-change library.

Comparison source: pinned Ruffle
`2372a4e148a67d68efd796f4477b3cddd4f880cd`,
`core/src/avm1/object/script_object.rs`, `set_local`, `add_property`, `get_data`.
This is a source comparison, not a Flash Player oracle. General casing/path,
original native-accessor mutation during its own conversion, complete Array/SWF
compatibility and all nine roadmap stages still require acceptance.

Validation / Проверено on Linux minimal: all 1093 CTest cases passed after
restoring lost generated-ELF execute bits and rerunning 27 affected cases. Three
old roots fixtures now install their coercible raw data through the existing
raw-data helper so destructive callbacks execute in the operation being tested,
rather than during the newly observable assignment. Their original callback
assertions remain active; the indexed setter callback still uses public assignment.
The final indexed-roots fixture was rebuilt and rerun successfully.

Extension v0.529 / Дополнение: original native-accessor writes now also select
the live descriptor after conversion, and deleted cached data is released from
GC roots. See [ARRAY_LENGTH_CACHE_GC.md](ARRAY_LENGTH_CACHE_GC.md).
