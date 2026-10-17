# Native methods and live Array length / Методы и актуальная length Array

v0.521: loaded native Array methods read existing own length data rather than
unconditionally reading the cached native Number. A missing/deleted property
means zero; inherited data and getters are ignored. A live builtin length
accessor still reads native state. Recreated ordinary String, Boolean, Null,
Undefined, Number and coercible Object data use AVM1 Int32 conversion. SWF4–6
ordinary name matching remains folded. Raw length data is retained until an
operation actually writes length. Arrays with a missing/nonbuiltin length
accessor leave numeric fast paths, including sorting snapshots.

В v0.521 нативные методы читают собственное сохранённое свойство length.
Удалённое/отсутствующее свойство означает ноль; наследуемые данные и getter
игнорируются. Штатный accessor читает внутренний Number. Заново созданные
String, Boolean, Null, Undefined, Number и Object преобразуются по AVM1 Int32.
В SWF4–6 сохраняется регистр обычных свойств. Исходное значение сохраняется,
пока операция не записывает длину. Несоответствующий length быстрый путь
заменяется общим, в том числе при сортировке.

Native length writes update the live builtin accessor or raw own data slot.
After deletion, push/unshift recreate an ordinary own length property. Writes
bypass setters/watchers and respect readonly data. Tail deletion uses only
old raw Number data, not a converted String/Boolean/Object or stale native
cache. Allocation errors during property recreation propagate; mutations already
published before an error are not rolled back. Incoming moved values are rooted
across length coercion callbacks, including counted strings and metadata.

Запись обновляет штатное хранилище либо обычный собственный слот; push/unshift
восстанавливают удалённое свойство. Сеттеры/watch не вызываются, readonly
сохраняется. Усечение использует только прежний Number, без преобразования
строк/Boolean/Object и без устаревшего кэша. Ошибки выделения памяти возвращаются
вызывающему; уже выполненные изменения не откатываются. Переносимые значения
защищены корнями GC во время преобразования length.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
avm1/object/script_object.rs length, set_length, set_element; globals/array.rs
join, slice, concat, reverse, sort and end operations. This source comparison
is not an original Flash Player oracle. Host behavior, public ABI and graph
format are unchanged. General indexed SetMember/C table growth after deletion,
sortOn field/option-list length, method-valued length data, virtual length
installation on native Arrays and direct nonnumeric builtin length assignment
remain open. The change does not certify complete Array compatibility or any
of the nine roadmap stages.

Сравнение исходников не заменяет Flash Player. ABI и формат графа сохранены.
Общие индексные записи после удаления length, длины списков полей/опций sortOn,
значение-функция в length, установка виртуальной length и присваивание
ненумерического значения штатному accessor остаются открытыми.

array_deleted_length covers nine methods across SWF4–8, C API, interpreter,
explicit/automatic JIT, graph roundtrips and ten deleted/recreated/inherited
length controls. Its flags variant checks readonly length and protected
elements; allocation checks OOM/GC/retry, property recreation and counted
string/metadata ownership. Its roots variant checks self-deleting collecting
coercion, errors and throws. Normal native calls retain their GC barrier; the
rooted direct native-callback route exercises actual collection inside coercion.
The main regression fails on v0.520 before the handler change.

Validation / Проверено v0.521 on Linux x64 C++98: minimal 1074/1074,
AGG 1093/1093 and Cairo 1094/1094 passed. Four executable permission failures
in each of AGG/Cairo were repaired and all eight reruns passed. Seven targeted
ASan/UBSan cases passed. Fresh Unicorn instruction/ABI checks passed 26 cases
for x86/ARM/Thumb/ARM64/RV32/RV64; x64 uses native JIT tests. Direct DT_NEEDED
audit passed for 1083 Cairo ELF artifacts, finding only libc, libgcc_s, libm
and libstdc++. Emulator checks and Linux results do not certify real foreign
OS acceptance or transitive/dlopen dependencies.
Все перечисленные проверки успешны; реальная приёмка остальных ОС и полная
совместимость SWF/Array остаются открытыми.

Follow-up v0.522 applies the live signed reader to sortOn field/option lists;
see [RU/EN contract](SORT_ON_LIVE_LENGTHS.md).
В v0.522 общий обработчик используется и для списков полей/опций sortOn.

Follow-up v0.524 applies live length to general indexed assignment and removes
loaded post-store cache growth; see [RU/EN contract](ARRAY_INDEX_LIVE_LENGTH.md).
В v0.524 обычные индексные записи используют текущую собственную длину.
