# Binary32 scene point arithmetic / Арифметика точки матрицы сцены

v0.338 adds two context-free C89 services in flashvm/matrix.h:
[fvm_matrix_point_twips](../include/flashvm/matrix.h#L28) and [fvm_matrix_inverse_twips](../include/flashvm/matrix.h#L43). [FVM_TwipMatrix](../include/flashvm/matrix.h#L10) has binary32
coefficients and signed 32-bit integer twip translations. These are distinct
from the raw SWF signed 16.16 coefficients in flashvm/fixed.h. Public long fields
carry signed32 values even on wider-long hosts; the struct is not a wire format.

Source inspection clarified the distinction: Ruffle's core/prelude.rs exports
render/src/matrix.rs Matrix for display objects; swf/src/types/matrix.rs describes
the serialized SWF representation. v0.337 raw fixed services remain valid for
that explicitly documented arithmetic, but are not a substitute for binary32
scene point application. All source references use pinned commit
2372a4e148a67d68efd796f4477b3cddd4f880cd.

Point application converts integer inputs to binary32, rounds each product and
the sum to binary32, rounds the linear term to nearest/even integer twips and
then adds integer translation modulo 2^32. Nonfinite linear terms become zero;
positive overflow and negative underflow become signed MIN. Volatile float
intermediates prevent fused/extended precision from changing these steps.
In particular signed MAX input loses its low bit in binary32 and identity
application maps it to signed MIN. Integer translation is added after rounding.

Inverse uses binary32 determinant/coefficient calculations, requires
abs(determinant)>FLT_EPSILON, and rounds translation to nearest/even twips.
Singular/NaN determinants return INVALID_ARGUMENT without changing output.
Both APIs require the checked binary32 float model and default FP environment;
unsupported float formats return UNSUPPORTED. They allocate nothing and need
no VM/context, bytecode, values, callbacks or GC. Aliased outputs are supported
as documented in the header and are published after all calculations.

Loaded point conversion now uses these services for final application and inverse.
Input pixel normalization still truncates/saturates to signed32 twips; getters
and inherited point fields remain excluded and old case policies are retained.
Failed loaded inverse preserves the normalized integer point directly, bypassing
binary32 point application. Host-only contexts keep their precise double path.
Root barriers, ordered x/y setters and failure restoration are unchanged.

Remaining: physical ancestor composition and source geometry snapshots still
come from FlashVM's existing double model. Translation preparation from that
model is not a full raw native scene storage policy. Drag, xmouse/ymouse, hit
queries and raster paths are not migrated here. No complete Flash Player,
scene/JIT/platform compatibility or roadmap-stage completion is claimed.

В v0.338 добавлены независимые C89-функции [fvm_matrix_point_twips](../include/flashvm/matrix.h#L28) и
[fvm_matrix_inverse_twips](../include/flashvm/matrix.h#L43). Коэффициенты [FVM_TwipMatrix](../include/flashvm/matrix.h#L10) имеют binary32, переносы
и координаты — signed 32-bit twips. Это другая модель, чем сырые 16.16 данные
SWF: сверка core/prelude.rs и render/src/matrix.rs уточнила различие.
Функции v0.337 не заменяют матрицу сцены автоматически.

Произведения и сумма округляются в binary32, линейный результат — до ближайшего
чётного целого twip, затем целый перенос прибавляется по модулю 2^32.
NaN/Infinity линейной части дают ноль, переполнение — signed MIN. Поэтому
signed MAX после float32-преобразования и единичной матрицы тоже даёт MIN.
Обратная матрица проверяет determinant по FLT_EPSILON и округляет перенос до
чётного twip. Промежуточные volatile float исключают FMA/расширенную точность.

Сервисы подключены к финальному применению матрицы и inverse загруженных
точек. При неудачной inverse сохраняется нормализованная целая точка, без
повторного float32-преобразования. Хостовый double-путь, корни, GC-барьер и
порядок setter-вызовов сохранены. Выходы при ошибках не меняются; VM-контекст
и аллокации сервисам не нужны.

Композиция предков и исходные geometry snapshots пока double; сырое хранение
сцены, drag, xmouse/ymouse и рендер не переведены. Это ограниченный шаг по
исходникам, без заявления полной совместимости Flash Player/JIT/платформ
или завершения стадий роудмапа.

Validation: Linux GCC 13.3 Release/default AGG passed 663/663 CTest tests.
matrix_twips is C89 and checks half/even rounding of both signs, combined versus
separately truncated terms, binary32 integer precision loss, modular translation,
signed endpoints, inverse translation/aliasing and invalid/range errors.
avm1_point_rounding covers host/SWF4–8, C/native entries and both directions.
Existing point/field/case/OOM tests passed with updated binary32 endpoint output
expectations; singular fallback separately preserves signed MAX before float loss.
fixed_matrix_no_alloc now also checks scene application/inverse and nonfinite terms.

Nine affected tests passed minimal Debug ASan heap/global + UBSan. Additional
exact-epsilon/twice-epsilon and NaN-determinant assertions passed Release and
that sanitizer profile after the full run. Stack checks and LeakSanitizer remain
disabled under the independently reproduced environment restriction. A standalone
C89 caller also linked/runs through cc with only the two C++98 scene implementation
objects and libm, without the VM library or an explicit C++ runtime. No other
platform/Cairo/Flash Player oracle run is claimed.

Проверки: 663/663 Linux Release/AGG, 9/9 затронутых тестов ограниченного
ASan heap/global + UBSan. Дополнительные точные границы epsilon, двойного epsilon
и NaN determinant повторно прошли оба профиля после полного прогона. Проверены
C89, host/SWF4–8, обе точки входа и направления, OOM/GC, чётные twips,
переполнение и сохранение signed MAX при singular fallback. Отдельный C89 caller
собран и выполнен без VM, только с двумя C++98 объектами и libm. Стек ASan,
утечки, другие платформы, Cairo и Flash Player здесь не проверялись.

## v0.339 per-ancestor composition follow-up

CLIP_WORLD_MATRIX.md adds binary32 composition and an explicit C world query.
Loaded point ancestry no longer composes the complete world matrix in double;
local source snapshots still use the existing native getter/double model.
The host point path remains double. Scroll rectangles/Stage and the other
subsystems listed above remain open.

В v0.339 композиция предков загруженной точки переведена на binary32 с
округлением каждого уровня; оставшаяся double-граница — локальные snapshots.
Хостовый путь не изменён. См. CLIP_WORLD_MATRIX.md.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_matrix_inverse_twips` | function | [include/flashvm/matrix.h:43](../include/flashvm/matrix.h#L43) |
| `fvm_matrix_point_twips` | function | [include/flashvm/matrix.h:28](../include/flashvm/matrix.h#L28) |
| `FVM_TwipMatrix` | struct | [include/flashvm/matrix.h:10](../include/flashvm/matrix.h#L10) |
<!-- source-links:end -->
