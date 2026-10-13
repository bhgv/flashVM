# BitmapData.perlinNoise

## Русский

v0.62 добавляет perlinNoise(baseX, baseY, numOctaves, randomSeed, stitch,
fractalNoise, channelOptions=7, grayScale=false, offsets=null).
Ненулевой baseX/baseY задаёт размер базовой волны: frequency=1/base.
Метод использует SVG-style gradient turbulence: четыре independently seeded
unit-gradient planes, общую permutation table, cubic interpolation, doubling
frequency и halving amplitude для последовательных октав. Fractal mode
суммирует signed noise, turbulence mode — абсолютные значения. Byte conversion
в fractal mode добавляет rounding offset до division by two.

Seed приводится к signed 32-bit; SVG normalization отличается от noise:
неположительный seed даёт magnitude modulo 2147483646 + 1, слишком большой
положительный seed ограничивается 2147483646. Генератор использует Schrage
reduction без 64-bit integer extensions и не меняет Math.random state.
Нулевой gradient pair получает (1,0), чтобы не делить на нулевую длину.

Color mask использует последовательно выбранные planes: пропущенный RGB
не расходует plane index. Grayscale использует plane 0 для всех RGB и plane 1
для выбранного alpha. Opaque alpha всегда 255; zero alpha очищает RGB в текущей
straight storage model. Base=0 отключает соответствующую частоту. Для stitch
частоты подгоняются к целому числу lattice periods на bitmap; границы октав
удваиваются. Реализовано SVG single-wrap lattice adjustment; произвольные
большие/отрицательные offsets не сертифицированы как seamless Flash behavior.

Все public numeric conversions и Point.x/y getters завершаются до native
storage borrow. Missing/null offset entries дают (0,0); каждый указанный entry
должен быть объектом текущего context. Затем повторно проверяется disposal.
Закрытый receiver возвращает -1; getter errors распространяются. Getter side
effects не откатываются. Complete tables и output buffer выделяются до записи;
только завершённый RGBA buffer публикуется swap, сохраняя managed identity.
Combined table/output heap scratch ограничен memory_limit_bytes. Callback 1550
append-only, переносится с классами без standard_install в receiver context.

Текущие защитные границы: 0..16 октав с floor fractional count, |base|≤1e6,
ненулевой |base|≥1e-6, |offset|≤1e6. Nonfinite base/count/offset отклоняются.
Это bounded implementation; полная Flash semantics вне этих границ открыта.

bitmap_perlin.cpp проверяет literal vectors от независимого SVG-style oracle
(4×3, seed 1, three octaves, turbulence/fractal), zero axes/octaves, seed 0,
channel packing/grayscale, opaque alpha, translated public offsets, stitch
boundary, quota rollback, invalid octave count, disposal из offset getter,
queue/file/loopback TCP и GC без standard globals. bitmap_perlin_allocation.cpp
инъецирует new/new[] failures в permutation/gradient/output staging, сверяет
старые pixels и allocation balance после уничтожения VM. Профили: headless,
AGG, Cairo, portable и heap/global ASan+UBSan. Stack ASan/leak scanning отключены
в текущем окружении.

Открыто: настоящий Flash reference corpus, getter/version/coercion edge cases,
точная premultiplied alpha quantization, большие/отрицательные stitch offsets,
большие workloads/cancellation и remaining pixelDissolve/filters/text/media.
SVG vectors подтверждают procedural arithmetic, не весь этап 4.

## English

Nonzero baseX/baseY select inverse frequencies; offsets shift pixel coordinates
independently for each octave.
v0.62 adds bounded Perlin turbulence and fractal sums using independent gradient
planes, shared permutations and octave frequency/amplitude scaling. Selected
color channels use consecutive planes; grayscale uses plane 0 for RGB and plane
1 for selected alpha. Seed normalization follows SVG, with signed-32-safe
Schrage arithmetic and an explicit unit fallback for zero-length gradients.
Math.random is unaffected.

Public offsets and numeric conversions precede native storage borrowing;
disposal is checked again afterwards. Table/output allocations precede complete
buffer swap, preserving managed identity and allocation rollback. Combined heap
scratch respects the configured limit. Stable callback 1550 survives queue/file/
network transport without global standard installation.

Current limits are sixteen octaves, bounded finite base/offset magnitudes and
nonzero base magnitude at least 1e-6. Stitch uses tile-adjusted frequencies and
SVG single-wrap lattice adjustment; large/negative offsets need Flash reference
conformance. Tests cover independent literal SVG vectors, fractal/turbulence,
axes/octaves, channel packing, grayscale/opacity, public offsets, boundary
stitching, quotas, getter disposal, transports/GC and injected allocation balance.
Headless, AGG, Cairo, portable and heap/global ASan+UBSan are checked with stack
ASan/leak scanning disabled. Historical Flash alpha/coercion/version conformance,
large workloads and remaining bitmap/filter/text/media work keep stage 4 open.

References: [SVG turbulence](https://www.w3.org/TR/SVG11/filters.html#feTurbulenceElement),
[AS2 BitmapData](https://open-flash.github.io/mirrors/as2-language-reference/flash/display/BitmapData.html).
