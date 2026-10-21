# Binary32 world matrix query / Запрос мировой матрицы binary32

v0.339 adds [fvm_matrix_multiply_twips](../include/flashvm/matrix.h#L31), a context-free C89 composition service
using separately rounded binary32 coefficient products/sums and nearest/even
translation twips followed by signed32 modular addition. Both input translations
must fit signed32; output can alias either input and is preserved on errors.
No VM, bytecode, callbacks, GC or allocation is involved in this numeric service.

[fvm_exec_clip_world_matrix](../include/flashvm/execution.h#L55)(context, clip, out) exposes a rooted value-based world
query for native clip instances. It reads native local geometry in physical
ancestor order, converts each local snapshot to binary32/twips and composes at
each level. The first local matrix is copied before ancestor multiplication;
the final scene-local identity composition is retained, including nonfinite
coefficient propagation. Cyclic/excessive ancestry is bounded by the existing
[FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS](../src/standard/movieclip_constants.h#L112) (1024). Errors preserve the output descriptor.
A temporary receiver root and native barrier protect old/mutated parent graphs
through getters; the result contains only copied numbers, no managed edges.
_lockroot does not shorten physical projection. The explicit query returns this
binary32 model even in an unloaded host context.

Loaded point conversion now calls this query, so ancestry, inverse and final
point application use the separate scene C services. Native host point conversion
retains its old double path. Point entry snapshots the loaded/host choice before
callbacks: a geometry getter can call public [fvm_load_memory](../include/flashvm/flashvm.h#L72), but cannot switch
mid-call to a different, uninitialized matrix representation. Input twip/float
preparation is shared in core/scene_numeric.hpp; units live in scoped matrix
constants rather than duplicated point-handler constants.

A three-level example distinguishes per-level arithmetic: three input translation
twips pass through two half-scales as 3 -> 2 -> 1, rather than being composed once
in double and truncated at the end. Nonfinite coefficient propagation is retained;
a resulting invalid inverse falls back to the original normalized integer point.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
render/src/matrix.rs multiplication and core/src/display_object.rs
local_to_global_matrix_without_own_scroll_rect/local_to_global_matrix. Local
source snapshots still use FlashVM's existing getter/double/decomposed display
model; they do not become raw renderer records. Scroll rectangles/Stage-specific
transforms, other display types, drag, mouse-property and raster migration remain
open. No full Flash Player equivalence, JIT or roadmap-stage completion is claimed.

В v0.339 добавлены независимое C89-умножение матриц [fvm_matrix_multiply_twips](../include/flashvm/matrix.h#L31)
и общий запрос [fvm_exec_clip_world_matrix](../include/flashvm/execution.h#L55). Коэффициенты считаются в binary32,
перенос округляется до чётного twip на каждом уровне и складывается по модулю
signed32. Числовой сервис не требует VM/байткода/GC/памяти; запрос мировой
матрицы удерживает клип корнем и защищает изменяемые графы нативным барьером.

Запрос проходит физических предков независимо от _lockroot; цикл ограничен
прежней границей 1024. Выход публикуется только после полного успеха и содержит
только числа. Загруженная точка использует новый запрос, хостовая точка остаётся
double. Выбор host/loaded фиксируется при входе: загрузка SWF из getter геометрии
не переключает вызов на другую, ещё не заполненную структуру матрицы.

Пример округления переносов: 3 twips через два масштаба 1/2 дают 3 -> 2 -> 1.
Сохранено распространение NaN/Infinity коэффициентов и fallback inverse.
Исходные локальные snapshots пока используют прежние getters и double-
декомпозицию; scrollRect/Stage, другие типы сцены, drag, mouse-свойства и рендер
не переведены. Это ограниченная сверка с исходниками Ruffle, без объявления
совместимости со всем Flash Player/JIT или завершения стадий.

Validation: Linux GCC 13.3 Release/default AGG passed 667/667 CTest tests.
execution_clip_world_matrix is C89: host/SWF4–8, explicit query, C/native point
parity, per-level half-even rounding, inverse roundtrip, root-lock independence,
nonfinite coefficient propagation/fallback and invalid arguments.
execution_clip_world_matrix_callbacks checks bounded parent cycles, unchanged
outputs on errors, getter throws/recovery and GC/root/native/script restoration.
execution_clip_world_matrix_allocation injects every query allocation failure;
the existing loaded point OOM sweeps exercise its nested C/native paths.
execution_clip_point_reload invokes real public [fvm_load_memory](../include/flashvm/flashvm.h#L72) from a geometry
getter through both entries and verifies stable host arithmetic and restored state.

matrix_twips and fixed_matrix_no_alloc now also check context-free composition,
including aliasing and no-new behavior. Additional dense/off-diagonal manual
oracle cases passed Release and sanitizer after the full run. Thirteen affected
tests passed minimal Debug ASan heap/global + UBSan; stack instrumentation and
LeakSanitizer remain disabled under the previously reproduced environment limit.
The current three scene numeric implementation objects also linked/run with a
standalone strict-C89 caller via cc/libm, without the VM or explicit C++ runtime.
No other platform, Cairo or Flash Player oracle run is claimed.

Проверки: 667/667 Linux Release/AGG и 13/13 затронутых тестов ограниченного
ASan heap/global + UBSan. Проверены C89, host/SWF4–8, нативные и C-вызовы,
поуровневые twips, inverse, _lockroot, NaN/Infinity, циклы, исключения, OOM и GC.
Отдельный тест реально загружает SWF из getter через публичный API. Плотная
матрица с перекрёстными коэффициентами повторно проверена в обоих профилях.
Три числовые единицы реализации собраны и выполнены с C89 caller без VM.
Стек ASan, утечки, другие платформы, Cairo и Flash Player не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_world_matrix` | function | [include/flashvm/execution.h:55](../include/flashvm/execution.h#L55) |
| `fvm_load_memory` | function | [include/flashvm/flashvm.h:72](../include/flashvm/flashvm.h#L72) |
| `fvm_matrix_multiply_twips` | function | [include/flashvm/matrix.h:31](../include/flashvm/matrix.h#L31) |
| `FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:112](../src/standard/movieclip_constants.h#L112) |
<!-- source-links:end -->
