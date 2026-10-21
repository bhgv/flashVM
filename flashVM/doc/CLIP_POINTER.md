# Clip-local pointer queries / Координаты указателя в клипе — v0.327

C89 [fvm_exec_clip_pointer](../include/flashvm/execution.h#L286)(context, clip, &x, &y) snapshots the world pointer and
projects it through the inverse physical native world matrix. It returns two
Number values; distinct non-NULL outputs are required. Context ownership,
instance type, singular/nonfinite transforms and output aliases are validated.
Errors preserve both outputs. The point snapshot precedes transform getters,
so a getter changing pointer state cannot mix two different input positions.

The clip is rooted and the existing geometry native barrier blocks collection
through transform getters. Allocation failures and script exceptions restore
root/native state. The physical ancestry walk now uses the separately documented
[FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS](../src/standard/movieclip_constants.h#L112) macro instead of a numeric literal.

MovieClip._xmouse and _ymouse use separate one-function adapters. The prototype
returns undefined; instances read the shared query. Stable callback IDs 1008 and
1009 fill unused slots without moving method IDs or changing graph format.
Transported accessors query the destination context's pointer, not a copied
source pointer. _lockroot does not change the world-space projection.

Indexed GetProperty/C display reads 20 and 21 use the intrinsic query without
looking up replaceable named mouse properties. Indexed writes and named accessors
are read-only. Indexed writes retain the existing legacy display service's
no-value-coercion policy, rather than adopting later Flash Player 21's extra
readonly coercions. Full historical player/version equivalence remains open.

Primary comparison: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/object/stage_object.rs x_mouse/y_mouse and readonly declarations.
Its action_property_coerce and activation.rs describe later player coercion
differences. This is source comparison, not independent Flash Player runtime
acceptance. Native doubles do not certify exact historical twip rounding.
Singular matrices return invalid argument; removed physical-vs-retained ancestry,
full mouse event delivery, hit targeting and drag remain open.

execution_clip_pointer is C89: host/SWF4–8 nested translation/scale/rotation,
lock independence, named/indexed reads and readonly writes, invalid/aliased
outputs, singular matrices, source method replacement and destination-pointer
channel transport. execution_clip_pointer_callbacks covers pointer mutation in a
matrix getter, blocked nested collection, exceptions and restored roots.
execution_clip_pointer_allocation injects every direct/named/indexed allocation
failure with collection between attempts and unchanged direct-query outputs.
avm1_pointer_properties exercises raw GetProperty/SetProperty bytecode, path
resolution, local values and legacy readonly coercion suppression in SWF4–8.

## Русский

Общий C-сервис возвращает локальную пару координат через обратную мировую матрицу
клипа. Позиция указателя фиксируется до вызовов геттеров трансформации; ошибка
сохраняет оба выхода. Нативные _xmouse/_ymouse и индексное чтение 20/21 используют
тот же сервис. Каждая операция-адаптер находится в отдельном файле. Добавлены
стабильные ID callbacks без сдвига прежних ID методов.

Транспортированный клип читает позицию своего нового контекста. _lockroot не
меняет проекцию. Проверяются вложенные трансформации, канал, подмена именованного
геттера, readonly, вырожденные матрицы, GC, исключения и отказы памяти.
Сохранена прежняя политика преобразований readonly-записи для старого профиля
Flash. Полная историческая точность, округление twips, удалённые предки,
мышиные события и drag остаются открытыми; стадии роудмапа не закрываются.

## v0.340: loaded scene arithmetic / арифметика загруженной сцены

Loaded SWF4–8 pointer queries now use the common C binary32 world, matrix
composition, inverse and integer-twip point services. The pointer and loaded/host
mode are captured before geometry callbacks. With the current unit device-pixel
ratio, global twips pass through device-space rounding before the inverse
local-twip projection; singular projections use identity in device-pixel space.
Unloaded hosts retain the strict double path and singular errors. Both outputs
remain atomic on failure. Named and indexed getters share this operation.

Для загруженных SWF4–8 запрос мыши использует общие C-функции матриц и точек:
коэффициенты binary32, целые twip, промежуточное округление экранных пикселей.
При необратимой проекции применяется единичная матрица в экранном пространстве.
Режим и положение мыши фиксируются до вызова getter. Для контекста без SWF
сохранены double и ошибка при вырожденной матрице.

Evidence: `execution_clip_pointer` covers SWF4–8, singular fallback, half/even
pixel rounding, host policy, named/indexed reads and transported getters.
Primary comparison: Ruffle `core/src/display_object.rs::local_mouse_position`,
commit `2372a4e148a67d68efd796f4477b3cddd4f880cd`. This is a source comparison,
not a Flash Player oracle. Stage view scaling/device-pixel ratios other than one
and full display-list integration remain open; no roadmap stage is closed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_pointer` | function | [include/flashvm/execution.h:286](../include/flashvm/execution.h#L286) |
| `FVM_MOVIECLIP_MATRIX_MAX_ANCESTORS` | macro / define | [src/standard/movieclip_constants.h:112](../src/standard/movieclip_constants.h#L112) |
<!-- source-links:end -->
