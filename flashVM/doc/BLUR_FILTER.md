# BlurFilter v0.66 / Размытие

Реализованы `flash.filters.BlurFilter`, наследование BitmapFilter,
конструктор, clone, blurX/blurY/quality, BitmapData.applyFilter и
BitmapData.generateFilterRect. Это проверенная математическая модель;
полная историческая Flash-сверка и MovieClip.filters остаются открытыми.

Defaults: blurX=4, blurY=4, quality=1. Конечные числовые параметры приводятся
публичным ActionScript conversion; blur ограничивается 0..255, quality —
floor с ограничением 0..15. Nonfinite значения отклоняются. Setter готовит
новое private numeric state целиком до замены; при conversion reentry остальные
параметры перечитываются, чтобы не потерять изменение вложенного setter.
Clone имеет независимое состояние и сохраняет captured class identity.

Алгоритм: centered separable box passes. Для ширины b>1 крайние веса равны
(b-(2*ceil((b-1)/2)-1))/2, внутренние веса — 1, сумма весов — b.
Ширина 0..1 — identity. Quality задаёт число горизонтально-вертикальных
проходов; 0 копирует без размытия. Скользящая сумма даёт линейный проход.
RGBA переводится во временные premultiplied double planes; alpha и RGB
фильтруются, затем результат возвращается в native straight RGBA. Вне
bitmap — transparent black. Padding radius*quality сохраняет промежуточное
размытие за границей исходника, а не обрезает его между проходами.

Source rectangle ограничивается source; затем его расширенный blur support
отображается в destination с clipping только по target. Полный source plane
участвует в вычислении halo, включая пиксели вне requested ROI, как в текущей
convolution-модели проекта. generateFilterRect расширяет public rectangle
на radius*quality по каждому направлению. Self-alias безопасен: все temporary
planes и готовый RGBA output создаются до публикации. Budget включает оба
planes и output; OOM не меняет pixels. Opaque target получает alpha=255.

Граф: instance/class kinds 60/61, callback IDs 1650..1657. Три private числа
проверяются при capture и decode; prototype/metatable/callback/indexed edges
в private record запрещены. Классы, getters/setters, clone и bitmap application
работают после queue/file/loopback TCP переноса в context без standard_install.
GC barriers сохраняют class factories, source/filter/receiver и временные графы.

Тесты blur_filter и blur_filter_allocation проверяют odd/even fractional
impulses, два прохода с независимыми весами, quality=0, opaque output,
expanded bounds, self-alias, reentrant setter, clamping/nonfinite rejection,
GC/transport, malformed private state, budget и последовательные отказы new
для application/setter/clone с rollback и allocation balance после teardown.

Открыто: Flash fixed-point/intermediate rounding, точные rules ROI/edges,
opaque premultiplied conversion, constructor/setter coercions для разных SWF,
reference bounds и сцена MovieClip/TextField/Button filters/cacheAsBitmap.
Ни класс, ни стадия 4 не объявляются полностью совместимыми с Flash.

Оригинальная документация Adobe (математика и публичный API):
https://open-flash.github.io/mirrors/swf-spec-19.pdf
https://open-flash.github.io/mirrors/as2-language-reference/flash/filters/BlurFilter.html

## English

v0.66 implements BlurFilter construction, inheritance, independent cloning,
blurX/blurY/quality, bitmap application and expanded output rectangles.
Defaults are 4/4/1. Finite blur values clamp to 0..255; quality floors/clamps
to 0..15. Nonfinite values fail. Reentrant setters refresh unrelated fields
before atomically replacing private numeric state; failed allocation preserves
old options.

Separable centered fractional box passes use sliding sums and premultiplied
double staging. Widths up to one are identity; quality zero copies unchanged.
Transparent padding preserves full intermediate support across repeated passes.
The source ROI is clipped before expansion; full-source pixels participate in
halo sampling. Destination clips the expanded output. Output bounds expand by
ceil((blur-1)/2)*quality for blur>1. Source aliases and allocation failures are
safe because both planes and final RGBA output are ready before publication.
The combined scratch budget is checked; opaque targets force alpha to 255.

Kinds 60/61 and callback IDs 1650..1657 preserve class/factory identity across
queue/file/TCP transfer. Canonical private records contain only three bounded
numbers. Imported filters and bitmap application work without global standard
installation. Tests cover literal odd/even impulse kernels, repeated passes,
identity/alpha/bounds, aliases/reentry, GC/transport, malformed state and OOM
rollback with teardown balance.

Exact historical Flash rounding, ROI/edge/opaque coercions and scene filters
remain open. This implementation does not certify complete BlurFilter or stage
4 conformance. The linked Adobe specifications describe box filtering and the
public parameter API; reference-player acceptance is still required.

Проверено / Verified: full headless 241/241, AGG 257/257, Cairo 257/257, portable 232/232; targeted ASan/UBSan 3/3. Leak detection and stack ASan are disabled in this environment. No actual Windows/macOS/Android execution certification.
