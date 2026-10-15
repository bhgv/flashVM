# Atomic native clip removal / Атомарное отсоединение клипа — v0.294

The existing native removeMovieClip implementation and forced scene removal now
prepare replacement maps before changing the live graph. Child-table removal,
current-name binding cleanup, clearing the parent edge and stopping playback
are published only after every allocation succeeds. Allocation errors return
FVM_OUT_OF_MEMORY and preserve both sides of ownership. Root chains are unchanged.

[GC:] Source maps retain all managed edges during preparation. Publication uses
only nonallocating swaps; no script or collection occurs after its first swap.
The existing native playback-stop value is now a documented uppercase macro.
No resource identity or graph schema changes.

Binding cleanup uses the complete counted stored name instead of truncating at
NUL. Other aliases remain intact. If the stored _name is missing, removal no
longer reads an uninitialized value. A depth entry is erased only if it still
refers to the object being removed: a stale wrapper cannot erase a replacement.
Native forced removal continues to support authored/negative depths, preserving
the previous host API and scene-control policy.

C89 fixtures cover host and loaded SWF4–8, counted-name bindings, retained aliases,
repeated removal and missing names. Allocation injection covers positive and
negative depths, parent/child/binding/playback preservation and GC between failed
attempts. A deliberately stale parent reference verifies replacement protection.
Existing depth and scene tests provide regression coverage.

This strengthens the existing native operation; Action RemoveSprite remains
absent from the dispatch catalog. Complete rename binding rules, historical
script removal/depth gating, removed descendants and unload-event/target-scope
behavior remain separate scene/interpreter work. No roadmap stage is closed.

Отсоединение клипа теперь публикуется после успешной подготовки всех таблиц.
При отказе памяти сохраняются родитель, слот глубины, именованная связь и состояние
проигрывания. Полное имя со встроенным NUL больше не усекается; чужой клип,
заменивший старый объект в слоте, не удаляется через устаревшую ссылку. Проверены
отрицательные глубины и GC между отказами. Это усиление нативной операции, а не
завершение семантики RemoveSprite, переименования или событий выгрузки.

## Counted replacement and scene rename — v0.295

Depth replacement and scene-driven rename now also locate the previous binding
using its complete counted name. A prefix alias is retained even when the old
name contains NUL. Replacement initializes the old-name value and checks the
lookup result, so a deleted _name cannot cause an uninitialized read. Cleanup
requires an object-valued binding to the replaced clip. Prepared ownership-map
publication and the existing API remain unchanged; no Action is added.

Regression fixtures exercise replacement in host and SWF4–8 modes, absent _name,
and a direct scene rename with both prefix and counted bindings. Heap/global
ASan and UBSan cover the two affected fixtures; stack/leak coverage is excluded.

Замена клипа на глубине и переименование из сцены теперь ищут прежнюю связь по
полному имени, включая NUL. Отдельная связь по префиксу сохраняется. Удалённое
_name больше не приводит к чтению неинициализированного значения при замене.
Это исправление существующей модели; полная семантика переименования остаётся
открытой, интерфейсы C и схема графа не меняются.

Validation: Linux AGG and Cairo each passed all 580 tests, including repaired
execution-permission launch failures (six AGG, four Cairo; no assertion failures).
The two affected sanitizer fixtures passed. All 26 accepted original SWFs matched
the pinned oracle in both renderer profiles. This does not certify full SWF or
cross-platform compatibility.

## Named depth encodings — v0.296

movieclip_constants.h now documents the native public depth bounds, child-slot
bias and SWF placement offset with separate uppercase macros. Creation, attach,
duplicate, lookup, swapping, legacy-slot migration, next-depth scanning and scene
placement use the same declared bounds without changing their accepted range.
The SWF offset and child-slot bias retain separate names because they represent
different encodings. Existing raw boundary and legacy snapshot fixtures remain
independent of the implementation macros. This is a readability change, not an
expansion of historical Flash depth support or a RemoveSprite implementation.

Границы нативных глубин, смещение внутренних слотов и преобразование глубины SWF
вынесены в подписанные макросы. Разные роли имеют разные имена даже при равных
значениях. Диапазон и формат сохранённых графов не изменены; граничные тесты
сохраняют исходные числовые значения как независимые проверки.

Validation for v0.296: Linux AGG and Cairo each passed 580 tests after repairing
launch permissions for four and ten executables respectively. All 26 original
SWFs matched the oracle in each profile. No semantic assertion failures occurred.
