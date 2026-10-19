# Intrinsic hit geometry / Общая геометрия hitTest — v0.320

C89 callers and generated bodies can invoke [fvm_exec_clip_hit_object](../include/flashvm/execution.h#L311)(context,
clip, target, out) to compare stroked native world bounds, or
[fvm_exec_clip_hit_point](../include/flashvm/execution.h#L313)(context, clip, world_x, world_y, shape, out) to test an
explicit world-space point against bounds or painted geometry. Each service has
its own implementation file under src/avm1/handlers. Neither accepts bytecode,
operand stacks, activation scopes or script method names. The output is a
Boolean value on success; errors preserve it. Empty geometry yields false.
Nonfinite point coordinates yield false. The existing painted-point coordinate
limit remains in effect; native bounds/paint recursion now has a documented
[FVM_MOVIECLIP_GEOMETRY_MAX_DEPTH](../src/standard/movieclip_constants.h#L81) macro.

Both services validate context ownership and reject ordinary objects and the
MovieClip class prototype. They root their native inputs and use the existing
native collector barrier while querying transforms and geometry. They do not
suspend enclosing barriers. Geometry property getters can run script; collection
inside those callbacks remains blocked, and collection between calls is covered.
Errors and allocation failures restore root and native-depth state.

Loaded MovieClip.hitTest now delegates to these services after its versioned
argument conversion, target resolution or unlocked-root point conversion.
The explicit C point API always uses world coordinates, including in loaded
contexts. It does not implicitly apply _root or _lockroot. Unloaded host script
calls keep their prior ordering and argument policy.

execution_clip_hit is a strict C89 fixture covering host and SWF4–8, transformed
world geometry, shape vs bounds, self/root overlap, invalid operands, and direct
operation after script hitTest/getBounds replacement. execution_clip_hit_allocation
injects every allocation failure in bounds-point, painted-point and object
queries, checking unchanged error output and restored scopes.
execution_clip_hit_callbacks checks transform getters, exceptions with unchanged
output, explicit rejection of nested collection and restored scopes in host/SWF4–8. Existing loaded
hit root/target callbacks and allocation fixtures exercise delegation.

These services expose existing native geometry, not full Flash geometry
compatibility. Detached ancestry, complete stroke/filter/mask semantics and
independent Flash Player acceptance remain open. They prepare a shared execution
interface; no machine-code JIT backend is introduced.

## Русский

Общие C-функции сравнивают мировые границы двух клипов либо проверяют мировую
точку по границам/рисунку. Интерфейс не зависит от байткода, стека или имён
скриптовых методов; каждая операция вынесена в свой файл. Загруженный hitTest
делегирует им после преобразования аргументов и координат корня. Прямой C-вызов
всегда принимает мировые координаты.

Проверяются владение контекстом и типы объектов; ошибки сохраняют выход.
Входные клипы удерживаются корнями GC, при обработке геометрии действует прежний
нативный запрет повторного входа в сборщик. Тесты охватывают host/SWF4–8,
подмену методов скриптом и отказы памяти. Ограничение рекурсии обозначено
смысловым макросом. Полная геометрическая совместимость и JIT остаются открытыми.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_hit_object` | function | [include/flashvm/execution.h:311](../include/flashvm/execution.h#L311) |
| `fvm_exec_clip_hit_point` | function | [include/flashvm/execution.h:313](../include/flashvm/execution.h#L313) |
| `FVM_MOVIECLIP_GEOMETRY_MAX_DEPTH` | macro / define | [src/standard/movieclip_constants.h:81](../src/standard/movieclip_constants.h#L81) |
<!-- source-links:end -->
