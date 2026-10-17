# Stage quality and ToggleQuality / Качество сцены

## Contract

The context-wide [`FVM_StageQuality`](../include/flashvm/stage.h#L11) policy defaults to [`FVM_STAGE_QUALITY_HIGH`](../include/flashvm/stage.h#L15).
LOW selects binary vector coverage. MEDIUM, HIGH and BEST currently use the
same backend-antialiased vector coverage. This is an explicit subset: different
bitmap downsampling and medium/best sampling grids are not implemented here.
The standalone context-free path/raster APIs keep their antialiased default.

The AVM1 ToggleQuality action (wire byte 0x08) consumes no operands and produces
none. HIGH or BEST becomes LOW. LOW or MEDIUM becomes HIGH, except that a BEST
preference retained by an earlier toggle restores BEST. Explicit host selection
resets this preference: selecting LOW after toggling BEST to LOW clears it.
The behavior follows the bounded model in the pinned primary
[Ruffle action implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/activation.rs).
This is not new acceptance evidence from an original Flash Player.

The policy belongs to the context, not a clip. It survives loading a movie into
that context; a fresh/worker context starts with HIGH. Copying/serializing a
MovieClip does not copy its source VM's stage policy. There are no managed GC
edges in this state. No process-global raster switch or thread-local override
is introduced. The API is usable with NO_GRAPHICS; rendering still returns
UNSUPPORTED when the selected profile has no raster backend.

## fvm_stage_get_quality

**Purpose:** inspect the current context's stage quality.

**Parameters:** `context` is the owning VM; `out` receives an [`FVM_StageQuality`](../include/flashvm/stage.h#L11).
Both pointers are required. **Results:** OK on success, INVALID_ARGUMENT for a
null pointer. Failed validation leaves the output unchanged. **Lifetime:** the
result is a copied enum, with no borrowed bytes or managed ownership.
**Execution:** call on the context's owning thread. No allocation, callback,
coercion or collection occurs.

## fvm_stage_set_quality

**Purpose:** explicitly select LOW, MEDIUM, HIGH or BEST.

**Parameters:** `context` is the owning VM; `quality` must be one of the four
named values. **Results:** OK; INVALID_ARGUMENT for a null context, invalid enum,
destruction, collection or collector-release phase. Invalid input leaves the
old policy/preference unchanged. **Lifetime:** input is copied immediately.
**Execution:** VM-thread calls, including ordinary running VM callbacks, are
allowed; cross-thread use and GC/destruction callbacks are not. It allocates
nothing and invokes no user code. BEST sets the remembered BEST preference;
all other explicit selections clear it.

## fvm_stage_toggle_quality

**Purpose:** apply the same state transition as the legacy AVM1 action.

**Parameters:** required owning `context`. **Results:** OK or INVALID_ARGUMENT
for null/destruction/collection/release state. It retains the BEST preference
while switching HIGH/BEST → LOW or LOW/MEDIUM → HIGH/BEST. **Lifetime:** no
returned handle or managed reference. **Execution:** same VM-thread restrictions
as the setter; no allocation, script callback, GC or replay. A body containing
ToggleQuality currently uses interpreter fallback when automatic JIT cannot
compile it; it is not silently skipped by the compiler.

## Renderer integration and limits

[`fvm_path_draw_quality`](../src/capi/path_c.cpp#L316) applies AGG's half-coverage threshold for LOW and Cairo's
ANTIALIAS_NONE. [`fvm_path_mask_quality`](../src/capi/path_c.cpp#L490) carries the same choice into gradient
and bitmap paint. Solid fills, strokes and optional geometry-coverage surfaces
use the same context selection. Bitmap paint's own smoothing option is retained;
edge antialiasing is a separate decision.

This work does not implement `_quality`/`_highquality` script properties, exact
historic quality grids, device renderer policies or multimedia. StopSounds
remains absent. No completion stage is closed by reaching 99 source dispatch
references. See the [roadmap](ROADMAP.md) and [dispatch inventory](AVM1_DISPATCH_INVENTORY.md).

## Русский

Качество — состояние всего контекста, по умолчанию HIGH. LOW выключает
сглаживание краёв векторной геометрии MovieClip: сплошных/bitmap/gradient заливок,
линий и coverage-масок. MEDIUM/HIGH/BEST пока используют одинаковое сглаживание
backend; различия bitmap downsampling и исторических сеток качества остаются
открытыми. Отдельные path API без контекста сохраняют прежнее сглаживание.

[`fvm_stage_get_quality`](../include/flashvm/stage.h#L19) копирует enum через обязательный выходной указатель.
[`fvm_stage_set_quality`](../include/flashvm/stage.h#L22) проверяет четыре значения и явно сбрасывает старое
предпочтение BEST при выборе любого другого режима. [`fvm_stage_toggle_quality`](../include/flashvm/stage.h#L25)
использует тот же переход, что реальный Action 0x08, и сохраняет предпочтение
BEST при временном переходе в LOW. Все операции без выделения памяти, callback
и GC; вызываются на VM-потоке, setter/toggle запрещены при сборке/уничтожении.
Ошибка аргументов не меняет состояние. Подробные параметры/статусы/lifetime
приведены в отдельных разделах функций выше.

Загрузка SWF не сбрасывает настройку хоста; новая VM получает HIGH. Перенос
объекта MovieClip не переносит настройку сцены его исходной VM. JIT-body с этим
Action пока исполняется через интерпретаторный fallback. Проверки охватывают
SWF1–8, независимые контексты, стек функции и четыре вида векторного рисования.
Полная совместимость Flash, script-свойства качества и StopSounds остаются
открытыми.

[Tutorial / пример](../examples/embedding/stage_quality/README.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_path_draw_quality` | function | [src/capi/path_c.cpp:316](../src/capi/path_c.cpp#L316) |
| `fvm_path_mask_quality` | function | [src/capi/path_c.cpp:490](../src/capi/path_c.cpp#L490) |
| `fvm_stage_get_quality` | function | [include/flashvm/stage.h:19](../include/flashvm/stage.h#L19) |
| `FVM_STAGE_QUALITY_HIGH` | enum constant | [include/flashvm/stage.h:15](../include/flashvm/stage.h#L15) |
| `fvm_stage_set_quality` | function | [include/flashvm/stage.h:22](../include/flashvm/stage.h#L22) |
| `fvm_stage_toggle_quality` | function | [include/flashvm/stage.h:25](../include/flashvm/stage.h#L25) |
| `FVM_StageQuality` | enum | [include/flashvm/stage.h:11](../include/flashvm/stage.h#L11) |
<!-- source-links:end -->
