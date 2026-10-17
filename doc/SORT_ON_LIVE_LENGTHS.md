# sortOn live list lengths / Актуальная длина списков sortOn

v0.522: loaded sortOn reads the field-name Array's own live Int32 length once.
A deleted, zero or negative length returns the receiver without converting
field names, option lengths or option values. All names are converted before
the option Array length is read. Options are used only when its own converted
length equals the captured positive field count. Missing/inherited lengths do
not revive native cached data. Recreated ordinary data and SWF4–6 folded
property spelling follow the same reader as other native Array methods.

В v0.522 sortOn один раз читает актуальную собственную Int32-длину массива
полей. Удалённая, нулевая или отрицательная length возвращает получателя без
преобразования имён и опций. Сначала преобразуются все имена, затем длина
массива опций; опции используются только при совпадении с сохранённым числом
полей. Наследуемые/отсутствующие данные не заменяются внутренним кэшем.
Восстановленное обычное свойство и регистр SWF4–6 читаются общим обработчиком.

The new private reader retains coercible length objects, propagates conversion
errors/throws and allocation failures, and publishes its signed output only
on success. The caller keeps the existing execution budget and flag parsing.
No public ABI or graph-format changes. Host policy is unchanged.
Частный обработчик сохраняет корни, возвращает ошибки/исключения/OOM и меняет
выходную длину только при успехе. ABI, формат графа и host-политика сохранены.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/globals/array.rs sort_on and object/script_object.rs length. This is source
comparison, not an original Flash Player oracle. General indexed writes after
length deletion, virtual/method-valued length and direct nonnumeric builtin
length assignment and the existing 64-field cap remain open, as do all nine
roadmap stages.
Сравнение исходников не заменяет Flash Player; оставшиеся правила length и
ограничение 64 полей и все девять этапов остаются открытыми.

array_sort_on_live_lengths checks nine field-list states against nine option-list
states in SWF4–8, C API, interpreter, explicit/automatic JIT and graph roundtrips.
Its roots variant checks the exact four-step conversion order, early no-op,
self-removal, detached captures, errors and throws. Ordinary native calls keep
the GC barrier; a rooted direct native-callback route performs actual collection.
Its allocation variant checks atomic reader output, invalid owners/contexts and
OOM/GC/retry. Sorting can publish partial writes before OOM; the retry fixture
restores its pinned input before retrying and does not assert rollback. The
main regression fails with the v0.521 implementation.

Проверки охватывают порядок преобразований, отсутствие лишних вызовов, GC,
исключения, перенос графа и отказы памяти. При повторе после OOM вход сортировки
восстанавливается; транзакционный откат не заявляется.

Validation / Проверено v0.522 on Linux x64 C++98: minimal 1077/1077,
AGG 1096/1096 and Cairo 1097/1097 passed, including successful reruns after
restoring executable permissions for five AGG and three Cairo binaries. Seven
targeted ASan/UBSan cases passed. Fresh direct DT_NEEDED audit passed for 1086
Cairo ELF artifacts, with only libc, libgcc_s, libm and libstdc++. These checks
do not certify transitive/dlopen dependencies or actual foreign-OS acceptance.
Реальные сборки/запуски остальных ОС и полная совместимость остаются открытыми.

Follow-up v0.523 removes the loaded 64-field cap and charges each field
comparison; see [RU/EN contract](SORT_ON_MANY_FIELDS.md).
В v0.523 предел 64 полей снят для загруженного SWF, добавлен учёт сравнений.
