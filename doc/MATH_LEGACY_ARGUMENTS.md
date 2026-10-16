# Math arguments / Аргументы Math — v0.194

В загруженных SWF4–8 каждый метод Math преобразует первые два аргумента,
слева направо, даже abs и random. Аргументы после второго не преобразуются;
min/max используют только два первых значения. Ошибка преобразования прерывает
вызов, в том числе до изменения состояния random. Numeric conversion использует
AVM1; отсутствующие аргументы преобразуются как undefined, затем в SWF4–6
применяется проверка arity. Исключения: random с любым числом аргументов,
min/max без аргументов и pow(1) допускаются. Незагруженный C API сохранён.

math_legacy_arguments проверяет callbacks/order, отсутствие третьего вызова,
min/max, ошибки abs/random, octal strings, недостающие аргументы и root/depth.
Глобальные isNaN/isFinite этим изменением не затронуты.

## English

Loaded Math methods coerce their first two operands in order, including unary
methods and random; later operands are ignored. min/max consume only the first
two. Conversion errors propagate before random state changes. AVM1 numeric
conversion handles missing operands as undefined; SWF4–6 then checks arity with
exceptions for random, empty min/max and pow(1). Unloaded host behavior remains.

The fixture checks callback counts/order, ignored third operands, min/max,
errors in unary/random methods, octal strings, missing arguments and restored
state. Reference: pinned Ruffle globals/math.rs method and parameters.rs get_f64,
commit 2372a4e148a67d68efd796f4477b3cddd4f880cd. Global isNaN/isFinite are unchanged.
Random distribution, exact rounding/transcendental compatibility, Math property
metadata and all roadmap stages remain open.

v0.481 adds loaded Math method/constant attributes and transactional first-load
activation; see [RU/EN rules](MATH_ATTRIBUTES.md). Numerical operations and the
host-derived constant values above are unchanged. Exact numerical compatibility
and all other acceptance work remain open.

В v0.481 исправлены атрибуты Math и первая загрузка из host-контекста;
порядок преобразования аргументов и числовые вычисления не менялись.
