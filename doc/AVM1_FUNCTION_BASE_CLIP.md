# Definition timeline / Таймлайн определения функции

v0.236 retains the loaded definition MovieClip in private function metadata.
Calling the function with a different receiver changes `this` while its display
commands and Function2 display preloads use the definition timeline. Shared
activation preparation selects and roots the timeline before preparing arguments
and registers; interpreted and attached native bodies inherit that same state.
Targets are restored after normal completion, exceptions and preparation failure.

В v0.236 функция сохраняет клип, в котором она была определена. Другой получатель
при вызове меняет `this`, но не таймлайн команд функции. Общая подготовка активации
используется интерпретатором и нативным телом; ссылка сохраняется при GC и передаче
через канал. Старые переданные функции без этого поля используют прежний fallback.

The managed edge is included in the existing private function graph without a
wire-layout or public [FVM_ScriptFrame](../include/flashvm/function.h#L16) ABI change. Native attachment is still
context-local and omitted from transfer. The fixture creates a closure on one
child and calls it on another, checks writes and STOP, attaches a native body,
collects, transfers the closure and verifies the interpreted fallback in a second
context. Receiver and target metadata restore to the suspended caller.

This implements retained clip identity for existing clips. Player path-based
resolution after removal/readdition, weak display references and deleted-base
execution termination remain open. No complete base-clip compatibility or roadmap
stage closure is claimed. Primary source: pinned Ruffle
2372a4e148a67d68efd796f4477b3cddd4f880cd, `core/src/avm1/activation.rs`
DefineFunction captures its activation base clip in the function record.

Validation: Linux AGG 438 tests (the new fixture passed after correcting its
receiving-context setup), Cairo Release 438 tests, and all 23 unchanged original
SWFs match byte for byte in each profile. Eight selected heap/global ASan+UBSan
checks pass; stack and leak instrumentation remain disabled. Non-Linux runtime
verification and all JIT backends remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ScriptFrame` | struct | [include/flashvm/function.h:16](../include/flashvm/function.h#L16) |
<!-- source-links:end -->
