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
