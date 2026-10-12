# BitmapData.loadBitmap

## Русский

v0.49 добавляет static BitmapData.loadBitmap(id). Linkage идентификатор
преобразуется в строку с сохранением полной длины (встроенный NUL не
превращается в совпадающий префикс). Ищется bitmap, экспортированный
ExportAssets текущей загруженной SWF-сцены. Возвращается новый объект с
независимой копией пикселей; исходный dictionary ресурс и другие результаты
loadBitmap не меняются при записи/диспозе этой копии. Отсутствующая связь,
не-bitmap symbol или disposed resource дают null.

При вызове из AVM1 выбирается ближайший MovieClip receiver активной цепочки
кадров. Это проверенная модель текущего вызова, а не заявление о полной
семантике всех loaded-movie/closure/library случаев Flash. Для вызова из C
без AVM1 receiver host должен привязать нужную сцену к глобальному _root.
Переданные через канал class и scene работают без standard_install в
принимающем контексте; class callback удерживает собственный prototype.
GC roots/barrier защищают сцену и class во время coercion и allocation.

Метод имеет стабильный native transport ID 1501. Старые IDs не меняются.
Проверены AVM1 CallMethod, приоритет текущей сцены над глобальным _root,
независимость копий, missing/NUL linkage, prototype override failure,
queue/file/loopback TCP и GC. bitmap_load_resource_allocation.cpp проверяет
каждую границу new/new[], неизменность исходных пикселей и баланс аллокаций
после уничтожения VM. Это проверка new/delete, а не полный LeakSanitizer.
Профили: headless, AGG, Cairo, portable и heap/global ASan+UBSan.

Открыто: точная resource scope семантика вложенных функций и загруженных
фильмов, external/shared libraries, все version/coercion edge cases,
BitmapData.draw и остальные операции, фильтры, текст и медиа. Этап 4 открыт.

## English

v0.49 adds static BitmapData.loadBitmap(id), returning an independent copy of
an exported bitmap dictionary resource. Full string length is preserved;
embedded NUL cannot accidentally match a linkage prefix. Missing/non-bitmap
or disposed resources return null. Copies retain the calling class's own
prototype and do not mutate the dictionary or other loaded results.

AVM1 calls resolve against the nearest active MovieClip receiver; C calls
without one use a host-bound global _root. This tested scope model still
needs full loaded-movie/closure/shared-library reference conformance.
Transferred class/scene graphs work without standard installation in the
receiver. Native transport ID 1501 is stable; existing IDs are unchanged.

Tests cover CallMethod, scope precedence, independent copies, missing/NUL
names, invalid prototype, GC and queue/file/loopback TCP. Allocation tests
inject new/new[] failures, preserve source pixels and verify new/delete
balance after VM destruction; this is not a complete leak scan. Headless,
AGG, Cairo, portable and heap/global ASan+UBSan profiles are checked.
Remaining draw/bitmap APIs, scope/version conformance, shared libraries,
filters, text and media keep stage 4 open.
