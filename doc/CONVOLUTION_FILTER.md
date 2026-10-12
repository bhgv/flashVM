# ConvolutionFilter

## Русский

v0.59 добавляет flash.filters.ConvolutionFilter, наследование BitmapFilter,
clone, matrix и свойства matrixX/matrixY/divisor/bias/preserveAlpha/clamp/color/alpha.
По умолчанию размеры ядра нулевые, divisor=1, bias=0, preserveAlpha/clamp=true,
замещающие color/alpha=0. Размеры приводятся к signed 32-bit и ограничиваются
0..15, color хранит low 24 bits, alpha ограничивается 0..1. Boolean flags
используют AS truth без numeric conversion. Коэффициенты, divisor/bias имеют
текущую защитную границу ±1 000 000; nonfinite значения отклоняются.

matrix getter каждый раз создаёт новый Array. Constructor/setter копируют
публичные entries; null/undefined даёт нулевые coefficients. Длина входной
таблицы должна быть целой 0..225; sparse undefined entries считаются нулём.
При недостатке entries ядро дополняется нулями до matrixX×matrixY. Уменьшение
размеров не укорачивает ранее сохранённую последовательность. При увеличении
добавляются нули. clone создаёт независимый private numeric record. Все
setter conversions завершаются до атомарной замены; после reentrant callback
параметры перечитываются, чтобы не терять изменения других свойств. OOM
сохраняет старый record; собственные side effects callbacks не откатываются.

BitmapData.applyFilter получает полный снимок источника, а не только sourceRect.
Это позволяет читать соседние kernel taps за границами ROI и поддерживает
source==destination. Destination clipping сохраняет смещения и допускает
sourceRect за пределами исходного bitmap: clamp повторяет ближайший край,
иначе применяется color/alpha. Ядро читается row-major с центром floor(size/2),
включая чётные размеры. Для каждого RGBA канала сумма произведений делится
на divisor и получает bias; divisor=0 при вычислении заменяется на 1. Текущая
модель использует straight RGBA, nearest byte rounding и 0..255 clamp.
preserveAlpha сохраняет центральный source alpha; opaque destination даёт
alpha=255, а zero alpha очищает RGB. Пустое ядро в текущей модели не меняет
пиксели. generateFilterRect возвращает независимый Rectangle без расширения.

Весь source snapshot проверяется по memory_limit_bytes до записи. После
выделения не вызываются getters и не выделяется память, поэтому OOM не
оставляет частичного результата. Shared factory capture сохраняет filter,
Array и Rectangle identities после queue/file/network transport без установки
стандартных глобальных объектов. Kinds 58/59 и callback ranges 1610..1613,
1630..1637, 1640..1647 добавлены без изменения прежних IDs. Private graph
проверяется до захвата и до восстановления: размеры, length/count, flags,
color/alpha, конечные coefficients; callbacks/prototypes/metatables запрещены.

convolution_filter.cpp проверяет defaults, scalar options, matrix snapshots,
clone/dimension padding, fixed 1D/2D/even kernels, edge replacement/clamp,
preserveAlpha, out-of-source ROI, halo taps, aliasing, quota rollback,
reentrant setters, queue/file/loopback TCP, GC, output bounds и malformed
serialized coefficients. convolution_filter_allocation.cpp инъецирует new/new[]
failures конструктора, matrix/dimension setters, getter/clone/apply и graph
restoration; сверяет прежние параметры/пиксели и allocation balance после VM.
Проверяемые профили: headless, AGG, Cairo, portable и heap/global ASan+UBSan.
Stack ASan/leak scanning отключены в текущем окружении; exhaustive graph
allocation tests имеют отдельный timeout 180s без сокращения failure coverage.

Открыто: эталонная Flash float32/premultiplied/fixed-point/coercion сверка,
все malformed argument corner cases, большие workload/cancellation, остальные
семь фильтров, MovieClip.filters/cacheAsBitmap, SWF filter lists и полный этап 4.
Наличие tested сценариев не означает завершения ConvolutionFilter conformance.

## English

v0.59 adds ConvolutionFilter, BitmapFilter inheritance, independent clone/matrix
snapshots and all eight scalar options. Defaults are an empty kernel, divisor 1,
bias 0, preserveAlpha/clamp true and transparent black edge replacement. Dimensions
use signed modular conversion clamped to 0..15, color keeps low RGB bits, alpha
clamps to 0..1 and flags use AS truth. Current finite coefficient/divisor/bias
safety bounds are ±1,000,000.

Public arrays are copied, with integer lengths 0..225 and zero-filled sparse or
missing entries. Dimensions extend kernels with zeros and do not truncate stored
sequences on shrink. Setters refresh options after reentrant coercion and publish
complete private records atomically. Failed allocation preserves prior coefficients;
callback side effects remain.

Bitmap application snapshots the entire source for halo taps and stable aliases.
Destination clipping permits out-of-source regions; taps repeat edges or use the
substitute color/alpha. Row-major kernels center at floor(size/2). Weighted straight
RGBA sums are divided, biased, clamped and rounded to bytes; zero divisor uses 1.
Preserved alpha comes from the central source sample, opaque targets stay opaque,
zero alpha clears RGB and empty kernels currently leave pixels unchanged. Filter
bounds remain unchanged. No callbacks/allocations follow source snapshot completion.

Captured factories and append-only IDs preserve queue/file/network transport.
Strict private graph validation checks counts, dimensions, bounded coefficients,
flags and edge color/alpha before restoration. Tests cover fixed kernel arithmetic,
edges/halos/overlap, independent copies, quotas, reentrant setters, transports/GC,
malformed state and exhaustive allocation rollback/balance. Headless, AGG, Cairo,
portable and heap/global ASan+UBSan are exercised; stack ASan and leak scanning
are disabled. Full Adobe float32/coercion/premultiplied conformance, other filters,
scene/SWF filters, large workload cancellation and roadmap stage 4 remain open.

API reference: [Adobe AS2 ConvolutionFilter mirror](https://open-flash.github.io/mirrors/as2-language-reference/flash/filters/ConvolutionFilter.html).
