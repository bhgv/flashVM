# Semantic numeric constants / Смысловые числовые константы

v0.230 replaces inline protocol/resource/flag/boundary values in the AVM1/SWF
execution path and related runtime clients with documented uppercase macros in
C89-compatible `.h` files. Each macro names its purpose; unrelated meanings keep
separate namespaces even when their numeric values match. Wire values, execution
command ABI, graph schema and existing behavior are preserved.

Значимые коды и границы вынесены в заголовки. В коде теперь, например,
[`FVM_AVM1_ACTION_INCREMENT`](../src/avm1/action_codes.h#L135), [`FVM_AVM1_ACTION_GET_MEMBER`](../src/avm1/action_codes.h#L131),
[`FVM_AVM1_F2_PRELOAD_ARGUMENTS`](../src/avm1/encoding_constants.h#L9), [`FVM_RESOURCE_MOVIECLIP`](../src/core/resource_kinds.h#L59) и
[`FVM_UINT32_SHIFT_MASK`](../src/core/numeric_constants.h#L23). Комментарий у каждого определения объясняет назначение.
Политика добавлена в AGENTS.md и применяется к дальнейшей разработке.

| Header | Purpose / Назначение |
| --- | --- |
| src/avm1/action_codes.h | Encoded AVM1 Action IDs and long-record boundary / коды Action и граница записей с payload. |
| src/avm1/encoding_constants.h | Function2/Try/GetURL2/GotoFrame2 flags, Push discriminators, recursion/register/call/literal/body bounds and poll cadence. |
| src/avm1/display_property_ids.h | Numeric GetProperty/SetProperty field IDs / индексы _x, _y, _alpha, _name и других полей. |
| src/core/resource_kinds.h | Stable managed object/resource IDs / типы объектов и ресурсов без смешения с кодами операций. |
| src/core/numeric_constants.h | Int32 wrapping, bit widths/masks, IEEE float decoding and AVM1 decimal-format policy. |
| src/swf/format_constants.h | SWF tag/version IDs, fill styles, lossless bitmap formats and short-tag encoding. |
| src/capi/graph_constants.h | Local/remote capability wire representations, separately from native resource kinds. |

The Action-to-execution-command translation remains in the interpreter; generated
code continues using the value/command C services instead of Action IDs. Existing
private descriptive enum aliases now draw resource values from the common header.
Plain counter initialization and traversal arithmetic do not gain meaningless
aliases. Third-party code and raw SWF/oracle fixtures remain unchanged.

Интерпретатор переводит Action в смысловые команды, а JIT-интерфейс сохраняет
независимость от пи-кода. Нули и единицы обычной инициализации/индексирования не
получают искусственных имён. Сторонние исходники и независимые fixtures сохранены.
Это выполненный проход по указанным подсистемам, а не утверждение, что весь
проект больше не содержит числовых литералов. Числовые параметры остальных
подсистем будут именоваться при дальнейших изменениях по этому же правилу.

Validation: all seven leaf headers compile together as strict C89. Linux AGG
full profile passes 432/432 tests/examples. A separate Linux Cairo Release build
passes 432/432. Ten selected tests pass minimal heap/global ASan + UBSan, with
stack/leak checks disabled under the documented environment restrictions.
The unchanged retrieved SWF<=8 corpus remains 17/22 byte-exact successful probes
in both graphics profiles; the same five failures remain recorded. No macro-only
change is presented as new SWF semantics or completion of a roadmap stage.

Проверены C89-заголовки, C++98-код и две реальные Linux-сборки. Реальных запусков
Windows/macOS/Android и генераторов JIT для шести архитектур ещё нет; пункт 9
и остальные общие стадии не закрыты. Эти макросы сохраняют значения и ABI,
поэтому состояние совместимости не повышается от одного переименования.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_AVM1_ACTION_GET_MEMBER` | macro / define | [src/avm1/action_codes.h:131](../src/avm1/action_codes.h#L131) |
| `FVM_AVM1_ACTION_INCREMENT` | macro / define | [src/avm1/action_codes.h:135](../src/avm1/action_codes.h#L135) |
| `FVM_AVM1_F2_PRELOAD_ARGUMENTS` | macro / define | [src/avm1/encoding_constants.h:9](../src/avm1/encoding_constants.h#L9) |
| `FVM_RESOURCE_MOVIECLIP` | macro / define | [src/core/resource_kinds.h:59](../src/core/resource_kinds.h#L59) |
| `FVM_UINT32_SHIFT_MASK` | macro / define | [src/core/numeric_constants.h:23](../src/core/numeric_constants.h#L23) |
<!-- source-links:end -->
