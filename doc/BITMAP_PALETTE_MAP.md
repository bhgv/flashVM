# BitmapData.paletteMap

## Русский

v0.56 добавляет paletteMap(source, sourceRect, destPoint, redArray,
greenArray, blueArray, alphaArray). Общие clipping и source snapshot правила
описаны в BITMAP_THRESHOLD.md. Для каждого source канала выбирается полное
32-битное значение из соответствующей таблицы; четыре вклада складываются
по модулю 2^32. Это позволяет переносить вклад между каналами и получать
переносы между байтами. Пропущенная/null/undefined таблица сохраняет исходный
канал. Opaque destination принудительно сохраняет alpha=255.

Все 256 публичных индексов каждой указанной таблицы читаются и приводятся
к числу до заимствования native storage. В текущей модели отсутствующий,
NaN или бесконечный элемент даёт нулевой вклад, конечное значение приводится
к младшим 32 битам. Таблицы могут иметь пользовательские getters; после них
повторно проверяется disposal источника и назначения. Getter side effects
сами по себе не откатываются. Если storage закрыт, возвращается -1.

Cache и source snapshot вместе ограничены memory_limit_bytes. После полного
snapshot нет callbacks или allocations: OOM не оставляет частичной записи,
а source==destination допускает смещённое перекрытие. Native callback 1545
сохраняется при переносе графа без standard_install и не требует graphics.

bitmap_palette_map.cpp проверяет перестановку каналов, identity/sparse tables,
modulo overflow, opaque alpha, self overlap, quota rollback, disposal из getter
и queue/GC. bitmap_palette_map_allocation.cpp инъецирует new/new[] failures,
проверяет исходные пиксели и баланс allocations после уничтожения VM.
Проверяемые профили: headless, AGG, Cairo, portable, heap/global ASan+UBSan;
stack ASan и leak scanner в этом окружении отключены.

Открыто: точная историческая Flash-семантика coercion/invalid palette,
premultiplied alpha quantization, оставшиеся bitmap/filter/text/media API.
Пункт 4 не завершён.

## English

v0.56 adds paletteMap with aligned clipping and complete source snapshots.
Each supplied table contributes a full 32-bit value indexed by its source
channel; contributions are summed modulo 2^32. Missing/null/undefined tables
preserve that channel. Opaque destination alpha remains 255.

All 256 public entries per supplied table are cached before borrowing native
storage. Current conversions map missing/NaN/infinite entries to zero and finite
values to their low 32 bits. Getter side effects are preserved; source and target
disposal is checked again afterwards and returns -1 for closed storage.
Combined cache/snapshot scratch respects the configured budget. No callbacks or
allocations follow snapshot completion, preserving overlap and OOM rollback.
Stable callback 1545 survives graph transport without global standard installation.

Tests cover sparse/identity palettes, channel swaps, modular carries, opaque alpha,
overlap, quotas, getter disposal, queue/GC and injected allocation rollback/balance.
Headless, AGG, Cairo, portable and heap/global ASan+UBSan are exercised; stack
ASan and leak scanning are disabled in this environment. Exact historical Flash
coercions and premultiplied storage, plus remaining graphics/text/media APIs,
keep roadmap stage 4 open.
