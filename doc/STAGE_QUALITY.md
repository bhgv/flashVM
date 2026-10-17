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
Since v0.557 ToggleQuality lowers to [fvm_jit_toggle_quality](../src/jit/display.cpp#L7), which directly
calls [fvm_stage_toggle_quality](../include/flashvm/stage.h#L25). Its source checkpoint precedes the transition;
the helper consumes no operands, pushes no result, allocates nothing and retains
no managed references. Unsupported operations elsewhere in a body can still
force fallback. Instruction-budget failure before the action leaves policy
unchanged; failure after delivery does not undo the transition.

## Script quality properties

`_quality` is a MovieClip accessor for context-wide state: its getter returns
LOW, MEDIUM, HIGH or BEST in uppercase. Its setter performs AVM1 string
conversion once, then accepts these four labels without regard to ASCII case.
Whitespace, unknown labels, embedded NUL suffixes and unsupported newer Flash
sampling labels leave state unchanged. Conversion failures propagate.

`_highquality` reads 2 for BEST, 1 for HIGH and 0 for LOW or MEDIUM. The named
setter ignores null/undefined; otherwise it performs AVM1 numeric conversion.
NaN is ignored, zero (including negative zero) selects LOW, values strictly
greater than 1.5 select BEST, and every other nonzero number selects HIGH.
Explicit script selection resets the remembered toggle preference just like
host selection. The class prototype itself returns undefined and ignores writes.

AVM1 GetProperty/SetProperty indices 16 and 19 use intrinsic state independently
of replaceable named properties. Index 16 always performs numeric conversion
before the typed setter, including null/undefined; therefore null selects LOW in SWF4–6 (zero) and leaves state unchanged
in SWF7–8 (NaN). Index 19 performs string conversion. Existing target/index
resolution happens before value conversion. Interpreter and automatic JIT use
the same execution helper. Native accessor identities are stable in graph
transport, and a transported clip observes its destination context's quality.

[GC:] Native invocation retains the receiver and arguments during named coercion;
explicit collection remains forbidden inside that native callback (and its error
propagates). The execution helper roots target/index/value and the resolved clip during
intrinsic coercion. No local snapshots or additional per-frame roots are added.
The resulting label is consumed without allocation or another callback.

The conversion policy follows pinned Ruffle
[stage properties](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs)
and [quality labels](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/render/src/quality.rs).
This source comparison is separate from original Flash Player acceptance.
Implementation: [`fvm_movieclip_quality_read`](../src/standard/movieclip_quality.cpp#L27), [`fvm_movieclip_quality_write`](../src/standard/movieclip_quality.cpp#L46) and
[`fvm_exec_display_property`](../include/flashvm/execution.h#L177).

### Свойства качества

`_quality` читает общее качество контекста строкой в верхнем регистре. Setter
однократно преобразует значение в AVM1-строку и распознаёт четыре имени без
учёта ASCII-регистра, но без удаления пробелов и без обрезания по NUL.
Неизвестное имя ничего не меняет; ошибка преобразования возвращается вызывающему.
`_highquality` читает BEST=2, HIGH=1, LOW/MEDIUM=0. Именованный setter игнорирует
null/undefined и NaN; числовой ноль задаёт LOW, значение строго больше 1.5 —
BEST, остальные ненулевые числа — HIGH. Прототип класса возвращает undefined
и игнорирует запись. Явный выбор сбрасывает прежнее предпочтение ToggleQuality.
Индексы AVM1 16/19 обходят подменённые именованные свойства. Индекс 16 сначала
преобразует значение в число: null задаёт LOW в SWF4–6 (ноль), но ничего
не меняет в SWF7–8 (NaN). Корни защищают
операнды и clip через вызовы пользовательских преобразований и GC. Явная
сборка внутри native-setter запрещена; её ошибка возвращается без изменения
качества. Перенос
callback сохраняет его назначение, но качество берётся из контекста получателя.
Оригинальный Flash Player и точные исторические sampling grids ещё не проверены.

## Renderer integration and limits

[`fvm_path_draw_quality`](../src/capi/path_c.cpp#L316) applies AGG's half-coverage threshold for LOW and Cairo's
ANTIALIAS_NONE. [`fvm_path_mask_quality`](../src/capi/path_c.cpp#L490) carries the same choice into gradient
and bitmap paint. Solid fills, strokes and optional geometry-coverage surfaces
use the same context selection. Bitmap paint's own smoothing option is retained;
edge antialiasing is a separate decision.

Exact historic quality grids and device renderer policies remain open.
StopSounds now stops [registered context outputs](AUDIO_CHANNELS.md); full SWF
audio remains open. A source dispatch inventory does not close any stage. See the [roadmap](ROADMAP.md) and [dispatch inventory](AVM1_DISPATCH_INVENTORY.md).

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
Action с v0.557 компилируется в прямой [fvm_jit_toggle_quality](../src/jit/display.cpp#L7) с общим stage
service. Он не меняет стек, не выделяет память и не содержит managed roots.
Checkpoint до действия может запретить его выполнение; ошибка бюджета после
действия не откатывает политику. Проверки охватывают
SWF1–8, независимые контексты, стек функции и четыре вида векторного рисования.
Полная совместимость Flash и SWF-аудио остаются открытыми; StopSounds
подключён к [контекстным аудиовыходам](AUDIO_CHANNELS.md).

## Native verification / Проверки JIT (v0.557)

`jit_stage_quality` uses isolated host/SWF1–8 contexts and interpreter, explicit
native and automatic native execution. It checks actual entries, all four
starting qualities over six toggles, remembered BEST, explicit LOW reset,
context isolation, skipped branches, lower operands and budget failures before
and after the state transition. `jit_stage_quality_allocation` denies allocation
for the direct helper and verifies unchanged policy in destruction, collecting,
release and null-context rejection. Existing `stage_quality` C89 checks now
verify actual native-entry increments, rather than expecting compiler rejection.

Тесты проверяют host/SWF1–8 и три режима с настоящими native entries, все
начальные уровни, BEST preference/reset, независимость контекстов, пропущенную
ветку, нижний операнд и бюджет до/после действия. Helper проверен без аллокаций
и при запрещённых lifecycle phases. Изменения не добавляют новые sampling grids,
bitmap downsampling, renderer/platform acceptance или полную Flash conformance.
`jit_stop_sounds` additionally executes both ToggleQuality and StopSounds in
one compiled function, preserving the lower operand while delivering actual
registered host stops. This covers interaction with the preceding v0.556 change.
Fallback fixtures now use unreachable byte 0x16, absent from the project's
bounded opcode catalog; executing that byte is not a supported Flash feature.

[Tutorial / пример](../examples/embedding/stage_quality/README.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_display_property` | function | [include/flashvm/execution.h:177](../include/flashvm/execution.h#L177) |
| `fvm_jit_toggle_quality` | function | [src/jit/display.cpp:7](../src/jit/display.cpp#L7) |
| `fvm_movieclip_quality_read` | function | [src/standard/movieclip_quality.cpp:27](../src/standard/movieclip_quality.cpp#L27) |
| `fvm_movieclip_quality_write` | function | [src/standard/movieclip_quality.cpp:46](../src/standard/movieclip_quality.cpp#L46) |
| `fvm_path_draw_quality` | function | [src/capi/path_c.cpp:316](../src/capi/path_c.cpp#L316) |
| `fvm_path_mask_quality` | function | [src/capi/path_c.cpp:490](../src/capi/path_c.cpp#L490) |
| `fvm_stage_get_quality` | function | [include/flashvm/stage.h:19](../include/flashvm/stage.h#L19) |
| `FVM_STAGE_QUALITY_HIGH` | enum constant | [include/flashvm/stage.h:15](../include/flashvm/stage.h#L15) |
| `fvm_stage_set_quality` | function | [include/flashvm/stage.h:22](../include/flashvm/stage.h#L22) |
| `fvm_stage_toggle_quality` | function | [include/flashvm/stage.h:25](../include/flashvm/stage.h#L25) |
| `FVM_StageQuality` | enum | [include/flashvm/stage.h:11](../include/flashvm/stage.h#L11) |
<!-- source-links:end -->
