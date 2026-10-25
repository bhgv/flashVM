# RemoveSprite native dispatch / Удаление клипа через RemoveSprite — v0.314

The interpreter now dispatches RemoveSprite to a common C89 service:
[fvm_exec_remove_target](../include/flashvm/execution.h#L438)(context, start, target). All inputs are explicit values;
the service reads no bytecode, operand stack or activation-selected target.
Native MovieClip values bypass conversion. Other values stringify once using the
loaded version policy and resolve through the existing native display path
resolver from the captured start clip. Missing/nonclip targets are ignored.
Resolved native instances use the v0.313 dynamic-depth/ancestor policy.

The interpreter supplies missing loaded operands as undefined; standalone host
underflow remains strict. It captures the path start before coercion. After a
successful operation, an effectively detached current target falls back to the
base clip, or null when the base itself is detached. The shared effective-state
helper walks the native parent chain with a documented cycle/nesting bound and
is also used by dynamic removal eligibility. The bytecode adapter alone changes
activation target state; the C service does not.

[GC:] Separate named root slots retain the path start, original input, converted
path and resolved clip across script coercion, GC and removal preparation. A
throwing conversion does not remove the target. The native ownership transaction
is atomic; a later interpreter target-state check may fail allocation after the
removal committed. Such a failure must restore caller activation/root chains and
preserve a coherent committed or uncommitted ownership graph, rather than claim
rollback of already completed script-visible work.

Fixtures:
- avm1_remove_sprite: host/SWF4–8 paths and native values (loaded register input
  avoids SWF4 GetVariable intentionally hiding MovieClips), negative/zero depth,
  missing operands, detached ancestor fallback and removal of the base clip.
- execution_remove_target: C89 direct calls without an activation, path/native
  values, ignored missing paths and invalid input ownership/pointers.
- execution_remove_target_callbacks: SWF4–8 collecting/throwing toString after
  deletion of its global input binding, unchanged C caller activation metadata.
- execution_remove_target_allocation: both path and native-value preparation.
- avm1_remove_sprite_allocation: fresh native graph at every injected boundary,
  root/execution restoration and coherent ownership before/after commit.

Primary comparison: pinned Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs action_remove_sprite and resolve_target_display_object;
core/src/avm1/globals.rs remove_display_object. The source dispatch audit now finds
95/100 catalogued Action references; five remain absent. A reference is not a
full semantic compatibility certificate.

Remaining limits: native MovieClip instances only, the existing smaller native
depth range, incomplete display-path/string policies, no unload event ordering,
no propagated historical removed flags and no full retained original-root model
for detached base clips. Broader function/scope behavior and complete SWF display
lifetime remain open. All nine roadmap stages remain open.

RemoveSprite подключён к общей C-службе с явной начальной целью и значением.
Поддерживаются нативный объект и преобразуемый путь, ограничения глубины,
недостающий undefined и восстановление выбранной цели после удаления предка.
Проверены SWF4–8, строгий host-режим, C89, GC внутри преобразования, исключения
и отказы памяти до и после фиксации удаления. Ветки интерпретатора теперь
найдены для 95 из 100 кодов каталога. События unload, полная модель display list,
исторические флаги удаления и сохранение исходного корня требуют дальнейшей
работы; ни одна стадия роудмапа не объявляется завершённой.

Validation: Linux AGG/Cairo each passed 610 tests. One AGG executable-permission
launch failure passed after restoring permissions; Cairo completed directly.
Five targeted heap/global ASan+UBSan fixtures passed (no stack/leak coverage).
All 26 accepted original SWFs matched the pinned oracle in each profile.
The factual Action-reference audit reports 95/100; --require-all still fails.

Follow-up v0.315: native detached instances now retain AVM1 parent/root provenance
for loaded execution, including descendant absolute paths and channel transport.
See MOVIECLIP_REMOVED_ANCESTRY.md; unload, lockroot and full lifetime remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_remove_target` | function | [include/flashvm/execution.h:438](../include/flashvm/execution.h#L438) |
<!-- source-links:end -->
