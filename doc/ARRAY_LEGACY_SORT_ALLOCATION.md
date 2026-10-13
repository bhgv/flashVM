# Sort allocation failures / Отказы памяти sort — v0.169

array_legacy_sort_allocation последовательно запрещает каждый new/new[] при
sort, UNIQUESORT, RETURNINDEXEDARRAY и sortOn в загруженном SWF8 до первого
успешного вызова каждого режима. Для каждого отказа создаётся отдельный контекст.
После результата разрешается память для очистки, проверяются restored GC frames,
native/execution/string depth, setter metadata, затем выполняется traced GC.
Отдельный счётчик C++ allocations возвращается к runtime baseline после каждого
context destroy и к нулю после runtime destroy. Он не заменяет проверку C malloc.

Indexed return не меняет receiver; успешный sort даёт ожидаемый порядок.
При OOM во время последовательной loaded copyback допустимы частичные записи:
каждая проверяемая позиция сохраняет старое или получает своё отсортированное
значение. Транзакционность не заявлена. Проверены обычные числа и объектные rank
поля, длина 4. Другие длины, пользовательские callback с OOM effects, native
callback GC barriers и все стадии остаются открытыми. ABI/wire неизменны.

## English

The test fails successive new/new[] boundaries for loaded SWF8 sort, UNIQUESORT,
RETURNINDEXEDARRAY and sortOn until each mode succeeds. Every boundary gets a
fresh context. Cleanup allocations are then allowed; GC frames, native/execution/
string depth and setter metadata must restore. Traced GC follows. C++ allocation
accounting returns to the runtime baseline after every context destruction and
zero after runtime destruction; this does not account for C malloc allocations.

Indexed return leaves its receiver unchanged. Successful sorting produces the
expected values. Sequential loaded copyback may partially write before OOM;
each checked position contains its original or corresponding sorted value.
Atomicity is not claimed. Tests use four primitive values/object rank fields.
Other lengths, callback OOM effects, native callback GC barriers and all roadmap
stages remain open. No public ABI/wire changes.
