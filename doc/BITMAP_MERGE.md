# BitmapData.merge

## Русский

v0.55 добавляет merge(source, sourceRect, destPoint, redMult, greenMult,
blueMult, alphaMult). Для каждого straight RGBA канала вычисляется integer
(source × multiplier + destination × (256 − multiplier)) / 256, с усечением
вниз. В текущей модели конечные коэффициенты ограничиваются 0..256 и
дробная часть отбрасывается; точная Flash-семантика вне диапазона требует
сверки. Opaque bitmap всегда сохраняет alpha=255. Это per-channel merge,
а не source-over композиция с premultiplied alpha.

Публичные геометрические/числовые параметры читаются до storage borrow.
Общие aligned clipping и snapshot правила описаны в BITMAP_THRESHOLD.md.
Весь eligible source сначала копируется, поэтому source==destination с
переносом не распространяет уже записанные значения. После snapshot нет
новых callbacks/allocations; OOM не оставляет частичной записи. При всех
нулевых весах не выделяется pixel scratch. Native callback ID 1544 сохраняет
совместимость остальных bitmap IDs и работу импортированного объекта без
global standard_install. Графический backend не требуется.

bitmap_merge.cpp проверяет half/selected/zero/full weights, integer rounding,
ограничение и дробные coefficients, opaque alpha, shifted self alias,
low-budget rollback, nonfinite arguments, queue/GC. bitmap_merge_allocation.cpp
инъецирует отказы new/new[] до публикации, сверяет исходные пиксели и
баланс после уничтожения VM. Профили: headless, AGG, Cairo, portable и
heap/global ASan+UBSan.

Открыто: полная premultiplied/Flash coercion сверка, остальные BitmapData
операции, filters, текст и медиа. Пункт 4 остаётся открытым.

## English

v0.55 adds per-channel merge using integer
(source × weight + destination × (256 − weight)) / 256. Finite weights are
currently clamped to 0..256 and truncated; exact out-of-range Flash behavior
still needs verification. Opaque alpha stays 255. Current arithmetic operates
on straight RGBA rather than source-over premultiplied composition.

Public region/weight coercion precedes native storage borrowing. Shared aligned
clipping and complete source snapshots preserve shifted aliases and failed
allocation rollback. No allocations/callbacks follow snapshot completion;
zero weights avoid pixel scratch. Stable callback ID 1544 works after graph
transfer without global standard installation or graphics backends.

Tests cover fixed weights/rounding/clamps, opaque alpha, overlap, budgets,
invalid numbers, queue/GC and injected allocation rollback/balance. Headless,
AGG, Cairo, portable and heap/global ASan+UBSan are checked. Exact Adobe
premultiplied/coercion behavior and remaining bitmap/filter/text/media APIs
keep roadmap stage 4 open.
