# Explicit clip resolution / Явное разрешение клипа — v0.319

The C89 [fvm_exec_resolve_clip](../include/flashvm/execution.h#L321)(context, start, input, allow_empty, out) service
accepts a native instance directly or stringifies another value once and resolves
its counted path from the explicit start. It uses versioned AVM1 coercion and the
existing native path resolver, including root locks. It does not use activation
scope, the interpreter stack or Action codes. Missing/nonclip targets return
undefined. Empty paths resolve to start only when allow_empty is enabled.
The output may alias input; errors preserve output. Temporary roots retain the
start, input and converted path across script conversion and property callbacks.
Remove-target now reuses this service with empty paths enabled.

Loaded MovieClip.hitTest uses this service for its single-argument overload,
with empty paths disabled. Missing targets and zero arguments produce false.
Geometry is queried after target coercion, so its side effects precede bounds
queries. Native object targets skip coercion. Unloaded host hitTest retains its
previous object/point and argument-error policy. Native geometry still blocks
collection during the native call; direct C resolution supports collection in
script conversion because its inputs are explicitly rooted.

Reference: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs resolve_target_display_object and
core/src/avm1/globals/movie_clip.rs hit_test. This source comparison is not an
independent Flash Player execution oracle. Full detached geometry, strokes,
filters, Unicode path case behavior and display-list compatibility remain open.

C89 movieclip_hit_target covers SWF4–8 paths, empty/missing targets, direct
objects, zero arguments, aliases and invalid input. movieclip_hit_target_callbacks
checks one toString conversion, exceptions, collection during direct C calls and
restored root/native/script scopes. movieclip_hit_target_allocation enumerates
allocation failures for direct aliased resolution and native path hitTest.

## Русский

Общий C-интерфейс принимает начальный клип и значение цели: нативный клип
используется напрямую, остальные значения один раз преобразуются в строку.
Поиск не зависит от активации, стека интерпретатора и кодов Action. Пустой путь
разрешается только при явном allow_empty; отсутствующая цель даёт undefined.
Вход и выход могут совпадать, ошибка сохраняет выход. Временные корни GC
удерживают входы при вызовах скрипта. Удаление цели использует тот же сервис.

В загруженном SWF одноаргументный hitTest принимает путь; пустая или отсутствующая
цель и отсутствие аргументов дают false. Геометрия вычисляется после преобразования
цели. Прежний режим host сохранён. Тесты проверяют SWF4–8, исключения, GC в прямом
C-вызове и отказы памяти. Это ограниченный шаг, не завершение MovieClip или стадий
роудмапа; полная геометрия удалённых клипов, обводок и фильтров остаётся открытой.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_resolve_clip` | function | [include/flashvm/execution.h:321](../include/flashvm/execution.h#L321) |
<!-- source-links:end -->
