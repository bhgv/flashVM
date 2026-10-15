# AVM1 execution services / Декомпозиция исполнения AVM1

Development checkpoint v0.226. The large handlers below moved from
`src/avm1/interpreter.cpp` into `src/avm1/handlers/`. Each implementation file
owns one service and its supported Flash-version variants; private helpers stay
with that service. The interpreter retains instruction framing, stack arity,
operand order, branches, register access, function-definition parsing and
exception-block orchestration. No roadmap stage is marked complete.

Контрольная точка v0.226. Крупные обработчики вынесены во вложенную папку
`src/avm1/handlers/`: один основной сервис на файл, рядом его приватные детали
и варианты поведения для версий Flash. В интерпретаторе остаются декодирование,
проверки/перекладка стека и управление потоком исполнения.

| File in handlers/ | Service | Retained semantics / Сохранённое поведение |
| --- | --- | --- |
| numeric.cpp | fvm_exec_numeric | Numeric arithmetic, remainder, 32-bit operations; coercion order, pre-SWF5 division error, SWF8 logical-shift result. |
| compare.cpp | fvm_exec_compare | Add2, loose/strict equality, Less2/Greater; primitive hints, coercion order, counted UTF-16 string ordering, NaN rules. |
| string_extract.cpp | fvm_exec_string_extract | StringExtract and MBStringExtract; length/start/text conversion order, Int32 wrapping, UTF-16 ranges and legacy ASCII gate. |
| member.cpp | fvm_exec_member | GetMember/SetMember, counted keys, raw native table index distinction. |
| display_property.cpp | fvm_exec_display_property | GetProperty/SetProperty, numeric index before target resolution, read-only gates and SWF7+ NaN name policy. |
| select_target.cpp | fvm_exec_select_target | Empty-path reset, undefined SWF6/7 boundary, invalid target state. |
| url.cpp | fvm_exec_url | Counted URL/target, target-before-URL conversion, flag validation and synchronous host sink. |
| trace.cpp | fvm_exec_trace | Conversion even without a sink; trace-specific undefined spelling and host barrier. |
| frame_control.cpp | fvm_exec_frame_control | NEXT/PREVIOUS/PLAY/STOP/GOTO commands; intrinsic MovieClip dispatch. |
| call_frame.cpp | fvm_exec_call_frame | Number/label/clip:frame resolution and nested actions without seeking. |
| script_call.cpp | fvm_avm1_call_script | Shared version-aware activation preparation: formals, arguments aliases, super, Function2 preloads, scopes and caller restoration. Private interpreter fallback only. |
| native_binding.cpp | fvm_function_set_body | Validate and attach/remove a context-local generated body and GC capture. |
| native_body.cpp | fvm_exec_native_body | Prepared-frame body entry, target/this restoration, result validation and finalizer-entry marker. Private activation dispatch. |

## Public C contract / Публичный C-контракт

`include/flashvm/execution.h` declares C89-compatible operations on explicit
`FVM_Value` operands and semantic command enums. They accept no Action opcode,
bytecode pointer, instruction offset or interpreter operand stack. Outputs may
alias inputs; failed value services leave the output intact. Runtime context
`FVM_Context` remains necessary for object ownership, GC, SWF-version policy,
display targets and host services. This is independence from the interpreter,
not independence from the managed runtime. Private `Execution` state is isolated
in `execution_state.hpp` and never appears in the public ABI.

C89-функции принимают значения и смысловые команды. Коды Action, пи-код,
счётчик команд и стек интерпретатора не входят в их интерфейсы. `FVM_Context`
нужен как среда объектов, GC, версионных правил и служб; он не передаёт
обработчику детали декодирования. Результат можно записать поверх операнда;
ошибка не меняет выходное значение. Изменения объектов, сделанные пользовательским
кодом до ошибки, не откатываются.

[GC:] Services validate same-context operands, then register mutable operand
and intermediate roots without changing implicit this. Host URL/trace callbacks
retain the ordinary native collection barrier. A generated body is a script
entry and may collect; its activation, arguments, receiver, preloads/registers,
function and capture are rooted. Extra generated-code temporaries must use
`gc_roots.h` or live register slots. There is no conservative machine-stack scan.

[GC:] Нативное тело работает как скриптовый вызов и может запускать GC. Его
активация, arguments, this, super, регистры, функция и capture удерживаются.
Дополнительные временные значения JIT обязан явно регистрировать через
`gc_roots.h` либо хранить в живых регистровых слотах. Стек процессора не сканируется.

## Generated body attachment / Привязка нативного тела

`function.h` introduces `FVM_ScriptFrame` ABI 1 and `FVM_ScriptBody`.
`fvm_function_set_body` keeps the original AVM1 body and metadata. Both bodies
share the activation builder, including Function2's separate receiver/implicit
this and suppress/preload policies. The descriptor exposes the actual arguments
object even when its named binding is suppressed, prepared register slots, and
undefined super when suppressed. Native code gets the prepared activation rather
than constructing another calling convention.

`fvm_function_set_body` сохраняет исходное тело и метаданные. При снятии привязки
исполняется исходный AVM1. Нативное тело получает готовую область видимости,
массив arguments, настоящий receiver, версионный implicit_this, super и регистры.
Объект arguments доступен в дескрипторе даже при подавленном именованном binding.
Код и userdata принадлежат хосту и должны жить до возврата всех активных вызовов.

[GC:] Each call snapshots its code/userdata/capture before activation setup;
self-unbinding affects later calls and cannot drop the running call's capture.
Capture is marked through the function and temporary snapshot roots. The body
returns ordinary `FVM_Result` statuses and uses `fvm_exception_raise` for script
throws; result strings are copied/validated before roots leave scope. Scope,
this, target, recursion depth and execution nesting restore on error/OOM.
An armed finalizer is consumed only after successful entry preparation.

Generated-code addresses, userdata and capture attachments are deliberately
absent from graph serialization. A transferred function keeps its script body
and executes it until the destination independently attaches generated code.
The wire schema is unchanged; the existing embedding ABI is unchanged apart
from additive APIs and the separately versioned new frame descriptor.

Перенос через канал сохраняет скриптовое тело, но не переносит адрес машинного
кода, userdata или capture привязки. После переноса функция интерпретируется;
при необходимости принимающий контекст компилирует её заново. Схема графа прежняя.

## Evidence and limitations / Проверки и границы

Linux GCC/G++ C90/C++98, default DEFS.txt (AGG, fonts, image codecs, SQLite,
OS ports and render jobs): all 426 CTest tests/examples passed. This includes
existing version/coercion/GC/target/frame/URL tests and the reference-runner's
generated FWS/CWS fixtures. New tests:

- execution_services.c: direct C calls without bytecode, SWF4–8 numeric policies,
  counted URL strings, trace, members, aliases and invalid arguments.
- script_native_body.c: prepared activation, capture collection, explicit script
  throw/recovery, self-unbinding, direct helper calls and channel fallback.
- script_native_function2.cpp: SWF5–8 preloads and paired suppression flags,
  receiver/this metadata, mutable register roots, finalizer entry and restoration.
- execution_services_allocation.cpp: injected allocation failures throughout
  shared activation/native entry and dynamic string addition; root/state recovery.

A NO_GRAPHICS/NO_FONTS/NO_IMAGE_CODECS/NO_SQLITE/NO_OS_PORTS profile passed ten
selected tests with ASan heap/global checks and UBSan. The restricted execution
environment cannot provide /proc access: stack instrumentation and LeakSanitizer
were disabled; runtime defaults were supplied by a separate validation-only
link object, not committed project source. No complete memory-safety claim.

Все 426 проверок полного профиля прошли. Отдельно прошли 10 проверок ASan/UBSan
минимального профиля. Проверки стека ASan и утечек отключены из-за ограничений
окружения. Исходный внешний 18-SWF корпус предыдущего сеанса отсутствует в этом
рабочем каталоге и не заявляется повторно проверенным. Сгенерированные FWS/CWS
fixtures runner проверены; это не независимая сверка с Flash Player.

A JIT backend is not implemented here. Generated instructions still need their
own instruction-budget accounting, safe points and source mappings; nested
interpreted calls preserve the shared budget and ordinary debugger/exception
state. Remaining interpreter-specific handlers can be moved behind the same
value contract as their scope semantics are stabilized. Existing legacy string,
MovieClip, Function2 and full SWF compatibility limitations remain open.

Этот шаг создаёт общий слой для будущего JIT, но не сам JIT. Учёт машинных
инструкций, safe points и привязка к исходникам нужны в backend. Оставшиеся
короткие обработчики и orchestration работают в интерпретаторе. Полная
совместимость Flash и платформенные стадии остаются открытыми.

### v0.317: intrinsic root-lock C service

clip_lock_root.cpp adds fvm_exec_clip_lock_root for explicit native instance/value
query and atomic assignment. It shares versioned Boolean conversion and roots
without accepting instruction bytes or an operand stack. Native getter/setter
bridges each occupy a separate standard file; root/path lookup reads raw native
state through movieclip_root_locked.cpp. See MOVIECLIP_LOCKROOT.md.

### v0.321–0.322: explicit sibling construction and CloneSprite

The stable duplicateMovieClip callback delegates to duplicate_clip.cpp's explicit
C value service. duplicate_target.cpp handles signed depth/name/source coercion
and resolution independently from AVM1 instruction encoding. CloneSprite's loop
adapter only prepares three operands, selects the explicit start and consumes
the request without pushing a result. Native instance preparation retains its
existing bridge and collector barrier; remaining construction decomposition and
full Flash duplicate semantics stay open. See DUPLICATE_CLIP_SERVICE.md and
AVM1_CLONE_SPRITE.md.

Сервисы дублирования принимают значения и объекты, без байткода и стека.
В интерпретаторе остаются подготовка операндов и выбор начальной цели.
Полная совместимость дублирования и остальные стадии остаются открытыми.

### v0.325: native construction dependencies in individual files

movieclip_copy_drawing.cpp and movieclip_sprite_class.cpp each contain one moved
function, shared by the native construction paths used through the explicit C
services. Their bodies are token-identical after symbol renaming; the private
movieclip_construction.hpp records root/barrier contracts. No script callback ID
or public ABI changes. See MOVIECLIP_CONSTRUCTION_DECOMPOSITION.md.

Два помощника создания клипов вынесены по одному в файл без изменения алгоритма.
Остальная декомпозиция и все стадии роудмапа остаются открытыми.

### v0.326–0.329: shared pointer and drag execution

Context pointer input, clip-local pointer projection and native begin/update/end
operations now have explicit C interfaces. StartDrag/EndDrag adapters retain only
operand preparation, encoded flag interpretation and selected-start handling;
coercion, path resolution, geometry and rooted drag lifetime use shared services.
See INPUT_POINTER.md, CLIP_POINTER.md, NATIVE_DRAG_SERVICE.md and AVM1_DRAG.md.

Координаты и drag используют общий C-слой без байткода и стека; в цикле остаётся
адаптация операндов. Полная совместимость и остальные стадии остаются открытыми.

### v0.330: native drag method adapters

The one-function movieclip_start_drag.cpp and movieclip_stop_drag.cpp adapters
share the native engine with the Action dispatches. drag_values.cpp prepares
versioned Boolean center and forward positional limits for C/native/generated
callers, without an interpreter stack. Existing callback IDs are preserved by
appending method entries. See MOVIECLIP_DRAG.md.

Скриптовые адаптеры вынесены по одному в файл; подготовка значений и движение
используют общий C-слой. Полная семантика Flash и стадии роудмапа остаются открытыми.

### v0.332: atomic local transform copy

clip_copy_transform.cpp provides fvm_exec_clip_copy_transform for native clip
identities, without Action IDs or an interpreter stack. A typed MovieClip.transform
setter shares it with C/generated callers; paired public/private map rollback
keeps matrix and color publication atomic on failures. See CLIP_COPY_TRANSFORM.md.

Общий C-сервис копирования локальной матрицы и цвета доступен будущему JIT и
setter MovieClip.transform. Исходные getter-побочные эффекты не откатываются;
публикация у получателя атомарна. Все стадии остаются открытыми.

### v0.333: explicit point conversion

clip_point.cpp exports fvm_exec_clip_point, used by separate local/global native
adapter files. It preserves existing point-field conversion, physical matrices,
ordered setter errors and callback transport IDs. See CLIP_POINT_SERVICE.md for
remaining loaded-Flash policy differences. The large MovieClip file no longer
owns the point operation; no instruction or operand-stack dependency was added.

Преобразование точки вынесено из MovieClip в общий C-сервис с двумя адаптерами.
Порядок преобразований, setter-ошибок и прежние ID сохранены; известные отличия
от Flash перечислены отдельно и не объявляются исправленными.

### v0.337: context-free arithmetic foundation

fixed_matrix_point.cpp and fixed_matrix_multiply.cpp in src/capi each own one
exported C89 operation, with shared unsigned limb helpers in core/fixed32.hpp.
Unlike value execution services these numeric primitives need no VM context,
roots or bytecode. They are available to future generated/scene code but do not
yet replace the native double matrix path. See FIXED_MATRIX.md.

Добавлены независимые числовые C-функции для будущего JIT и сцены. В каждой
единице реализации одна экспортируемая операция; текущая double-сцена пока
не изменена, стадии роудмапа остаются открытыми.

### v0.338: binary32 scene services

matrix_point_twips.cpp and matrix_inverse_twips.cpp each own a context-free C89
operation in src/capi. clip_point.cpp uses them for loaded final application and
inverse; host point conversion remains double. Separate binary32 rounding helpers
and scoped macros retain no execution state. See SCENE_MATRIX.md for representation
clarification, fallback rules and remaining double ancestry/snapshot limitations.

Добавлены независимые C89 application/inverse матрицы сцены binary32; общий
обработчик загруженной точки делегирует им числовую работу. Контекст, байткод,
стек и GC числовым сервисам не нужны. Остальные стадии не закрываются.

### v0.339: explicit world snapshot and scene composition

matrix_multiply_twips.cpp owns context-free numeric composition; clip_world_matrix.cpp
owns the rooted physical world query shared by C/generated callers and loaded
point conversion. Safe scalar preparation moved to core/scene_numeric.hpp.
Neither service accepts bytecode, Action IDs or an operand stack. Host double
point behavior and existing callback serialization IDs remain unchanged.
See CLIP_WORLD_MATRIX.md for snapshot/cycle/error contracts and remaining scope.

Общий запрос мировой матрицы и числовое умножение доступны C/JIT отдельно от
байткода и стека. Переносимость, сырая сцена и завершение стадий ещё открыты.

In v0.341, host pointer projection and drag inverse projection share the
context-free C89 `fvm_affine_inverse_point` service (AFFINE_PROJECTION.md).
The execution adapters retain state, callbacks, GC barriers and publication.
В v0.341 числовая double-проекция мыши/drag вынесена в общий C-интерфейс;
состояние и корни остаются в адаптерах. Это не завершает этапы roadmap.

In v0.342, host point conversion delegates inverse construction and point
application to context-free C services, preserving its distinct evaluation
order and IEEE propagation; see AFFINE_PROJECTION.md. В адаптере остаются
чтение/запись свойств, версия SWF и GC; числовые операции доступны JIT.

In v0.343, Array/Object literal construction moves to separate value-based C
services; only count/operand decoding and stack publication remain in the
interpreter. Full input snapshots and partial results are GC-rooted across
callbacks. See LITERAL_SERVICES.md. Построение литералов вынесено из основного
цикла; C-интерфейс не принимает стек или пи-код.

In v0.344, `fvm_exec_literal_count` extracts versioned count conversion as well.
The adapter handles undefined rejection without consuming entry operands; it
still owns encoded layout and stack publication. Преобразование счётчика,
построение Array и построение Object теперь доступны через отдельные C-функции.

In v0.345, NewObject count preparation delegates to `fvm_exec_constructor_count`,
preserving its distinct host bound and loaded call-count semantics. The obsolete
literal-limit alias leaves the bytecode header. См. NEW_OBJECT_NAMES.md;
адаптер сохраняет порядок имени/счётчика и отвечает только за раскладку аргументов.
