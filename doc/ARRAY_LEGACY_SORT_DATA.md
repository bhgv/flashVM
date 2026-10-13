# Generic sort raw data / Данные общего sort — v0.157

Загруженный общий sort/sortOn использует own signed length и raw данные
элементов, без getter/prototype lookup. Snapshot заполняется всеми позициями,
включая собственные undefined вместо holes. Запись результата использует raw
own data без index setter/watcher, учитывая flags; method field getter/comparator
по-прежнему отдельный пользовательский код. Незагруженный путь не меняется.

[GC:] Snapshot удерживается allocation-free root через sort callbacks и запись
результата. Work accounting включает length coercion, input/copyback позиции и
AVM1 callbacks; полный учёт стоимости comparator merge ещё открыт.

array_legacy_sort_data проверяет SWF6..8, borrowed Object, virtual length/index,
prototype holes, readonly data, отсутствие index callbacks/watch и данные после
GC. Native fast sparse sort ещё имеет отдельное поведение holes. Callback timing,
все sort flags/return rules, native C accessor raw data, case/version и стадии
остаются открытыми. Generic cap 65536 остаётся; ошибка copyback может оставить
предыдущие изменения. ABI/wire не меняются.

## English

Loaded generic sort/sortOn reads own signed length and raw element data without
index/length getters or prototypes. Snapshot materializes every position,
including own undefined holes. Raw copyback skips index setters/watchers and
honors own flags. SortOn field getters/custom comparators remain user callbacks.
Unloaded behavior is unchanged. An allocation-free root retains the snapshot
through callbacks and copyback. Shared budget includes length coercion, input/
output positions and AVM1 callbacks; full merge comparison charging remains open.
SWF6..8 tests cover borrowed objects, virtual length/index, inherited holes,
readonly data, no index callbacks/watch and post-GC data. Native fast sparse
holes, callback timing, all flag/return rules, native C accessor data, case/version
and every stage stay open. Generic cap remains 65536; failed copyback may leave
prior changes. No public ABI/wire changes.

## v0.159: undefined comparison / Сравнение undefined

Загруженный sort больше не помещает undefined принудительно в конец:
встроенное сравнение использует AVM1 строку (пустую до SWF7, "undefined"
с SWF7), а custom comparator получает undefined как обычный аргумент.
Исправлена проверка v0.157 для SWF6: пустой ключ расположен перед числами.
Незагруженный режим сохраняет прежний контракт. Sparse native arrays и
точный порядок callback остаются открытыми.

Loaded sort no longer forces undefined to the end. Builtin comparison uses
the versioned AVM1 string (empty before SWF7, "undefined" from SWF7);
custom comparators receive undefined operands. The v0.157 SWF6 fixture now
expects the empty key before numeric strings. Unloaded behavior is preserved.
Sparse native arrays and exact callback order remain open.

Проверки v0.159: AGG 326/Cairo 325, по пять профильных тестов portable и
ASan/UBSan; 15 исходных SWF совпадают побайтово. Отказы запуска повторены
после восстановления executable bits. Полный ASan/UBSan прогон v0.158
(308 тестов) завершён; два отказа запуска повторно прошли на v0.159.
Ограничения sanitizer: heap/global ASan и UBSan; stack ASan/leak checks выключены.

v0.159 validation: AGG 326/Cairo 325, five focused portable and ASan/UBSan
checks, 15 byte-exact original SWFs. Launch failures passed after restoring
executable bits. The full v0.158 sanitizer run (308 tests) completed; its two
launch failures passed on v0.159. ASan covers heap/globals; stack and leak checks
are disabled. This evidence does not close any roadmap stage.
