# Double inverse projection / Обратная double-проекция

v0.341 adds `flashvm/affine.h` and `fvm_affine_inverse_point`. The C89 API
accepts an `FVM_Affine`, a point or vector mode, two numbers and two distinct
output pointers. It has no VM context, managed values, bytecode, allocations,
callbacks or GC dependencies. Generated code may call the same symbol as hosts.

Point mode subtracts the matrix translation; vector mode ignores it. Both
evaluate the inverse linear transform in double precision. Zero/nonfinite
determinants and nonfinite results return `FVM_INVALID_ARGUMENT`, preserving
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
`fvm_affine_inverse_point`. Он принимает только матрицу и числовые данные,
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

`fvm_affine_invert` and `fvm_affine_transform_point` now supply the unloaded-host
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
