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
