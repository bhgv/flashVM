# Поиск __gc без allocation v0.74 / Allocation-free finalizer lookup

Ранее temporary fvm_detail::string("__gc") выделял память после установки
finalized flag. OOM оставлял финализатор непозванным, но retry его пропускал.
Та же ошибка могла выйти C++ exception через C [fvm_context_destroy](../include/flashvm/flashvm.h#L70) и прервать
cleanup. Regression сначала воспроизводит потерянный native callback.

Lookup теперь перебирает уже существующие native/property entries и сравнивает
counted key длины 4. Allocation/callbacks отсутствуют; embedded-NUL/suffix keys
не совпадают. Стоимость O(числа metamethods + properties метатаблицы), вместо
временного выделения ключа для hash lookup. Native callback invocation, включая
его ошибку/исключение, остаётся once-only; side effects не откатываются.

Script dispatch дополнительно переводит C++ bad_alloc в OUT_OF_MEMORY и другие
исключения в ERROR внутри finalizer bridge. **Script pre-entry OOM ещё требует
отдельной проверки**: эта правка не сертифицирует retry до первого Action-кода.
Финализатор C без собственных выделений вызывается при allocator allowance=0
во время context destruction; сама функция destruction не имеет error result.

gc_finalizer_lookup_allocation перебирает каждый new failure при collection,
проверяет retry/one call и teardown allocation balance; отдельно запрещает все
new при native context destruction и проверяет canonical counted-key matching.
Общая приёмка GC и всего debugger остаётся открытой.

## English

Temporary finalizer lookup keys could allocate after the finalized flag was set,
losing a native callback on OOM and potentially unwinding C++ through C context
teardown. Lookup now scans existing metatable entries without allocating and
matches exactly four counted bytes; NUL/suffix lookalikes do not qualify.
Cost is linear in metatable entries. Native invocation stays once-only, including
callback failure; executed effects are not rolled back.

Script dispatch now translates C++ allocation/unexpected exceptions inside the
finalizer bridge. Script preparation OOM before the first Action still requires
separate retry acceptance; this change does not certify it. A native callback
without its own allocations runs under zero-allocation teardown; the void destroy
API cannot report an error result.

Failure injection covers every collection new boundary, native retry/exactly-once
invocation, allocation-free native teardown, canonical key matching and final
allocation balance. Full GC/debugger acceptance remains open.

Проверки v0.74 / verification: full headless 256/256; targeted
GC/metatable/resource/debugger/text tests 52/52 in AGG, Cairo, portable and
ASan/UBSan. Existing feature exclusions apply: fonts in AGG/Cairo only,
TCP disabled in portable; heap/global ASan and UBSan active, LSAN/stack
ASan disabled. Linux profiles only; complete stage 7 remains open.

## Обновление v0.75 / v0.75 update

Script pre-entry OOM дополнительно проверен и исправлен в v0.75. Invocation entry
markers, once-only boundary и retry: [GC_FINALIZER_PREPARATION.md](GC_FINALIZER_PREPARATION.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_context_destroy` | function | [include/flashvm/flashvm.h:70](../include/flashvm/flashvm.h#L70) |
<!-- source-links:end -->
