# Frame request preparation / Подготовка запроса кадра — v0.306

The C89 execution API exposes [fvm_exec_frame_request](../include/flashvm/execution.h#L336)(context, input, with_bias,
scene_bias, output). It prepares a Number or counted String without moving a
timeline, selecting a target or reading bytecode. GotoFrame2 now delegates its
previous conversion block to this single-operation handler; decoding remains in
the interpreter. Generated bodies can combine this service with the existing
explicit-clip frame-control service.

| Request | Preparation |
| --- | --- |
| Number | Preserve its value without string conversion |
| Other value | Perform versioned string conversion once |
| Nonempty all-ASCII-decimal-digit string | Parse as Number |
| Other counted string | Preserve as frame label, including embedded NUL |
| Number with with_bias enabled | Add the validated scene-frame offset |
| Label with with_bias enabled | Preserve label without applying numeric offset |

The existing digit-only policy is deliberately retained: +2, -1, 2.5, leading
whitespace and exponent strings remain labels. Full historical GotoFrame2 parsing,
label/path/scene behavior and clip lifecycle semantics still require independent
compatibility work. This extraction does not certify their completeness.

The scene offset is bounded by [FVM_EXEC_MAX_SCENE_FRAME_BIAS](../include/flashvm/execution.h#L14). It is validated
before callbacks. with_bias is independent of the offset's numeric value: enabled
zero bias performs the existing addition and can change negative zero to positive
zero; disabled bias preserves negative zero. No target or playback state is
changed by preparation alone. Invalid inputs and allocation/coercion failures
leave output unchanged, including when output aliases input.

[GC:] The service roots the original value and prepared text across toString,
collection and exceptions. It does not introduce a native collection barrier.
The caller must retain successful output across later collections. Interpreter
roots retain the prepared frame while the current target is resolved after
coercion; no stack or Action identifier crosses the C interface.

Timeline payload widths, direct-frame base conversion, scene-offset position and
temporary frame-root layout now use documented uppercase macros in private
headers. They are separate from the semantic public offset bound and commands.

Loaded Call and GotoFrame2 also prepare undefined when their operand is absent.
Their existing versioned conversion then applies. The raw-byte fixture uses
explicit first/second/undefined frame labels, avoiding accidental empty-label
matches. SWF4–6's empty undefined string finds no such label; SWF7–8's literal
undefined selects the named frame. Malformed Call/GotoFrame2 payloads still fail
before semantic request preparation. Host-only missing-stack errors remain.

Tests: execution_frame_request is C89 and covers host/SWF4–8, numeric/label rules,
counted NUL, aliases, bias bounds and zero signs. Callback tests collect during
one toString, reject valueOf use and verify exception/root restoration. Allocation
injection checks aliases through every failure. avm1_frame_missing checks loaded
empty operands, scene-bias labels, decimal requests and malformed payloads.

The pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd activation.rs uses missing
pop undefined for frame requests. This bounded source comparison supports the
underflow change; it does not certify the retained FlashVM request parser as
complete or replace the original-SWF oracle corpus.

Подготовка кадра вынесена из цикла в отдельный C89-сервис без байткода и стека.
Он сохраняет прежнее преобразование значений в число или полную строковую метку,
применяет смещение только к числу и удерживает корни при toString, GC и исключениях.
Выбор цели и перемещение таймлайна остаются отдельными операциями. Для пустого
стека Call/GotoFrame2 загруженного SWF подставляется undefined; повреждённый payload
отвергается до вызова сервиса. Подписаны макросы разметки и границ. Полная семантика
кадров, все JIT backend и этапы роудмапа остаются открытыми.

Validation: Linux AGG/Cairo each passed 592 tests after repairing launch permissions
for eleven AGG and six Cairo executables. Four new heap/global ASan+UBSan fixtures
passed (no stack/leak coverage). All 26 accepted original SWFs matched the pinned
oracle in both profiles. An initial compile failure identified a missing standard
API declaration include; the build was corrected before any tests were accepted.
The bounded Action-reference audit remains 94/100, with no stage-completion claim.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_frame_request` | function | [include/flashvm/execution.h:336](../include/flashvm/execution.h#L336) |
| `FVM_EXEC_MAX_SCENE_FRAME_BIAS` | macro / define | [include/flashvm/execution.h:14](../include/flashvm/execution.h#L14) |
<!-- source-links:end -->
