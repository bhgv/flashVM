# Double inverse projection / Обратная double-проекция

v0.341 adds `flashvm/affine.h` and [`fvm_affine_inverse_point`](../include/flashvm/affine.h#L22). The C89 API
accepts an [`FVM_Affine`](../include/flashvm/path.h#L39), a point or vector mode, two numbers and two distinct
output pointers. It has no VM context, managed values, bytecode, allocations,
callbacks or GC dependencies. Generated code may call the same symbol as hosts.

Point mode subtracts the matrix translation; vector mode ignores it. Both
evaluate the inverse linear transform in double precision. Zero/nonfinite
determinants and nonfinite results return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L47), preserving
both outputs. Output pointers may alias matrix fields: results are staged
before either write. Nonfinite translations are ignored in vector mode.
The operation follows the current default floating-point environment; it does
not promise a correctly rounded mathematical inverse for extreme matrices.

The unloaded-host pointer handler and native drag update now share this
operation. Drag state, callback ordering, root scopes, revision checks,
constraints and atomic matrix publication remain in the execution adapter.
Loaded pointer queries still use the separate binary32/twip services. Drag
still uses the existing double policy: this extraction does not establish
exact Flash drag rounding or complete Stage/device scaling.

В v0.341 обратное преобразование точки/смещения вынесено в общий C-интерфейс
[`fvm_affine_inverse_point`](../include/flashvm/affine.h#L22). Он принимает только матрицу и числовые данные,
не использует VM, GC, байткод, callbacks и выделение памяти. Обработчики
перетаскивания и мыши host-контекста используют одну функцию; её также может
вызывать будущий JIT. Режим точки учитывает перенос, режим вектора игнорирует.
Ошибки сохраняют оба результата, допускается запись в поля исходной матрицы.
Округление drag, совместимость сцены и этапы roadmap остаются открытыми.

Evidence: `affine_inverse_point.c` is a standalone C89 caller covering dense
matrices, point/vector parity, output aliasing, null/mode validation, singular
matrices, NaN, ignored vector translation and arithmetic overflow.
`fixed_matrix_no_alloc` calls both modes with global allocation disabled.
The existing drag and pointer suites cover adapter behavior, callback errors,
GC roots, cancellation/replacement, constraints and allocation failures.

## v0.342: raw affine operations / Базовые affine-операции

[`fvm_affine_invert`](../include/flashvm/affine.h#L38) and [`fvm_affine_transform_point`](../include/flashvm/affine.h#L30) now supply the unloaded-host
`localToGlobal`/`globalToLocal` adapter. They preserve its existing evaluation
order: inverse coefficients, inverse translation, then point application.
Unlike strict inverse-point projection, raw algebra propagates IEEE nonfinite
results; inversion only rejects a zero/NaN determinant. Invalid pointer arguments
preserve outputs; both services support output aliasing and allocate nothing.

This distinction is observable: for x scale 3, translation 1e16 and global x
1e16+2, inverse-then-apply produces 1 on the tested binary64 platform, while
subtract-then-divide produces 2/3. The adapter retains the former behavior.
This is regression evidence for the host policy, not a Flash compatibility claim.
Loaded SWFs continue through the binary32/twip path. Ordered field setters and
GC/root handling remain in the adapter.

В v0.342 преобразование точки и построение обратной double-матрицы вынесены
в отдельные C-функции без VM и выделения памяти. Сохранены порядок вычислений,
распространение Infinity/NaN и последовательность setter в host-контексте.
Для загруженного SWF по-прежнему используется binary32/twip. Тесты
`affine_operations` и `execution_clip_point_host_numeric` проверяют алиасы,
ошибки, переполнение и потерю точности при больших переносах; общий тест без
выделения памяти вызывает обе новые функции.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Affine` | struct | [include/flashvm/path.h:39](../include/flashvm/path.h#L39) |
| `fvm_affine_inverse_point` | function | [include/flashvm/affine.h:22](../include/flashvm/affine.h#L22) |
| `fvm_affine_invert` | function | [include/flashvm/affine.h:38](../include/flashvm/affine.h#L38) |
| `fvm_affine_transform_point` | function | [include/flashvm/affine.h:30](../include/flashvm/affine.h#L30) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
<!-- source-links:end -->
