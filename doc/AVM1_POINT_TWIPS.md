# Loaded point input twips / Входные координаты точки в twips

v0.335 normalizes stored numeric point inputs in loaded SWF4–8 before applying
the native coordinate matrix. Multiply pixels by twenty, truncate toward zero,
and saturate to signed 32-bit twips; NaN becomes zero, positive/negative infinity
saturate to the corresponding endpoint. The bounded integer is kept in double,
so implementation needs neither C++11 nor long long, and never performs an
out-of-range floating-to-integer cast. Representation constants are documented
macros in clip_point_constants.h. Host-only contexts keep precise double inputs
and their existing rejection of nonfinite coordinates.

Primary source inspected: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
swf/src/types/twips.rs Twips::from_pixels and core/src/avm1/globals/movie_clip.rs
local_to_global/global_to_local. This is a source-based input conversion step,
not a complete Flash Player equivalence claim. In particular the native world
matrix still uses double composition/application; SWF fixed-point matrix products,
wrapping translation and exact output-twip arithmetic remain open. Historical
case-folded point spelling and raw display-matrix snapshots also remain open.

В v0.335 собственные числовые координаты точки загруженных SWF4–8 переводятся
в signed 32-bit twips: двадцать twips в пикселе, усечение к нулю, насыщение
по границам. NaN даёт ноль, бесконечности — соответствующие границы. Расчёт
не использует long long и не приводит выходящий за диапазон double к целому.
Константы представления вынесены в пояснённые макросы. Хостовый режим сохраняет
точность double и прежний отказ для NaN/Infinity.

Это сверка входного преобразования с исходниками Ruffle, без запуска Flash
Player. Матрицы сцены пока считаются в double: точные fixed-point произведения,
переполнение переноса и округление результата в twips ещё не реализованы.
Старое сопоставление регистра имён и сырые снимки матрицы тоже остаются открытыми.

Validation: Linux GCC 13.3 Release/default AGG passed 657/657 CTest tests,
650 initially and seven permission-denied launch failures after restoring those
executables. avm1_point_twips independently checks decimal truncation, both signs,
NaN, infinities, DBL_MAX and signed endpoints for SWF4–8, both directions and
both C/native entry paths. It also verifies unchanged host double/nonfinite rules.
Five affected tests passed minimal Debug ASan heap/global + UBSan; the previously
reproduced environment restrictions still exclude stack instrumentation and
LeakSanitizer. No Flash Player, other platform or Cairo run is claimed here.

Проверено: 657/657 Linux Release/AGG, с восстановлением прав семи файлов и
повтором только соответствующих запусков; 5/5 затронутых тестов ограниченного
ASan heap/global + UBSan. Десятичные значения, знаки, NaN/Infinity, DBL_MAX,
границы signed 32-bit и сохранение хостового режима проверены независимо.
Матрицы fixed-point, Flash Player, другие платформы и утечки не заявляются.

## v0.338 final application follow-up

SCENE_MATRIX.md records binary32 final application/inverse after input twip
normalization. Identity application of signed MAX input now produces signed MIN
because binary32 first rounds the input to 2^31; the old double-path output
expectations above are historical. Ancestor/source scene preparation remains open.

В v0.338 финальные application/inverse используют binary32 и чётные twips;
старые выходные ожидания double-пути выше исторические. Представления сырых
SWF-матриц и матриц сцены различаются. См. SCENE_MATRIX.md.
