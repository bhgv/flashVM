# Method-valued Array length / Значение-метод в длине Array

Implemented in v0.525. The shared loaded Array own-data length reader now
recognizes a native-method slot as its lazily wrapped Function value, as
Function.apply already does. It reuses the stable method wrapper/cache, so a
script override of that Function's valueOf participates in Int32 conversion.
It never calls the length method as a length getter. Own stored data and own
accessors retain precedence; inherited methods remain ignored. SWF4–6 uses
legacy case folding, while SWF7–8 requires exact spelling. Cold wrapper
allocation errors propagate through length reading and raw native length
mutation; failed public-private length reads preserve the caller's output.

This extends the raw-data representation to native C API method slots. Normal
script Function values stored as data already use the existing object coercion
path. The reference is Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
ScriptObject get_data/length/set_length and native Array helpers. This source
comparison is not an original Flash Player oracle.

[GC:] The wrapped Function is rooted across valueOf, even when conversion deletes
its own method/cache entry and global receiver binding. Errors and throws
propagate without resuming the requested index write. Ordinary native calls
retain their GC barrier; the explicitly pinned direct native-callback fixture
also checks real collection during conversion.

В v0.525 общий обработчик собственной длины загруженных Array учитывает
нативный метод как его Function-объект, используя существующий кэш обёрток.
Переопределённый valueOf этой функции участвует в преобразовании Int32; сам
метод длины не вызывается как getter. Данные и accessor имеют прежний приоритет,
методы прототипа игнорируются. В SWF4–6 регистр имени несущественен, в SWF7–8
требуется точное length. Ошибки выделения обёртки передаются вызывающему коду;
результат неуспешного чтения длины не публикуется. Обёртка защищена при удалении
метода, его кэша и глобальной ссылки внутри собирающего valueOf.

Tests / проверки: array_method_length covers cold/warm wrappers, mixed-case
and inherited controls, join/slice/push and indexed writes, SWF4–8, C API,
interpreter, explicit/automatic JIT and serialized graph roundtrips followed
by GC. Its roots variant exercises collecting self-deletion, errors/throws,
and pinned direct native callbacks. Its allocation variant sweeps cold-wrapper
OOM for the signed reader, join/slice and indexed assignment, then collects
and retries, checking unchanged output on reader failure.

No ABI or graph-format change. Remaining virtual length semantics, nonnumeric
assignment to the original built-in accessor, general Array/SWF compatibility,
and all nine roadmap stages remain open. Linux checks do not certify actual
Windows/macOS/Android runs.
ABI и формат графа сохранены. Полная семантика виртуальной length,
ненумерическое присваивание штатному accessor, совместимость Array/SWF и
все девять этапов остаются открытыми; приёмка других ОС не подтверждена.

Validation / Проверено v0.525 on Linux x64 C++98: minimal 1086/1086,
AGG 1105/1105 and Cairo 1106/1106 passed after restoring executable permissions
on three/two/seven test files respectively; all permission-only reruns passed.
Seven targeted ASan/UBSan cases passed. Fresh Unicorn instruction/ABI checks
passed 26 cases for x86/ARM/Thumb/ARM64/RV32/RV64; native x64 uses host JIT.
Direct DT_NEEDED audit passed for 1095 Cairo ELF artifacts with only libc,
libgcc_s, libm and libstdc++. Transitive/dlopen dependency auditing and native
foreign OS acceptance remain open. Все перечисленные проверки успешны;
приёмка других ОС и завершение девяти этапов не подтверждены.
