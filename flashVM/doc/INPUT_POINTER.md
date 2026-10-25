# World pointer state / Мировая позиция указателя — v0.326

C89 [fvm_input_set_pointer](../include/flashvm/input.h#L91)(context, x, y) publishes a finite native world/stage
pixel position without delivering an event. [fvm_input_get_pointer](../include/flashvm/input.h#L96)(context, &x,
&y) reads the last accepted pair, initially (0,0). Read outputs must be non-NULL
and distinct. Invalid operations preserve the coordinate pair or both outputs.
Each context owns independent state; these APIs are owning-thread only.

Direct input dispatch updates the pointer after validation and button mapping,
before invoking the host callback. The callback therefore sees its event's
position. Callback errors do not roll pointer state back. Queuing alone does
not publish a position: the queued event updates it only when dispatched by the
pump. Invalid events/phases never publish their coordinates. Raw mouse-motion
hosts can use the setter without manufacturing recognized gesture events.

Coordinates are native world pixels, before per-clip transforms. They do not
create mouse events, perform hit targeting or drag, or add a JIT backend. The
position is context state rather than a MovieClip graph property; transporting
an object does not copy a source context's pointer into the destination.
The state is two scalars with no managed edges, allocation or extra GC roots.

input_pointer is strict C89 and covers initial/independent contexts, error-output
preservation, alias rejection, finite extremes, NaN/infinity rejection, enqueue
vs delivery, callback visibility and invalid-event preservation.
input_callback_failures also checks visibility inside throwing callbacks.
Existing input, queue and recognizer scenarios remain enabled.

## Русский

Добавлены C-функции явной записи и чтения позиции указателя в мировых пикселях.
Начальная позиция — (0,0), у каждого контекста своя пара. Ошибка сохраняет
состояние или оба выхода; адреса выходов должны быть разными. Событие публикует
позицию после проверки и до callback, даже если callback затем возвращает ошибку
через исключение C++. Постановка в очередь не меняет позицию до доставки.

Это основа относительных координат и drag, без заявления их полной реализации.
Тест C89 проверяет изоляцию контекстов, ошибки, конечные пределы, NaN/бесконечности,
разницу между очередью и доставкой и видимость координат в callback. Состояние
содержит только числа и не требует новых корней GC.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_input_get_pointer` | function | [include/flashvm/input.h:96](../include/flashvm/input.h#L96) |
| `fvm_input_set_pointer` | function | [include/flashvm/input.h:91](../include/flashvm/input.h#L91) |
<!-- source-links:end -->
