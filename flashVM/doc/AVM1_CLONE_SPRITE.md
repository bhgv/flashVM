# CloneSprite and explicit duplication / CloneSprite и явное дублирование — v0.322

The AVM1 dispatcher recognizes CloneSprite (0x24), consumes source/name/depth
and pushes no result. Loaded missing operands are padded below existing values;
unloaded host execution remains strict. The adapter selects an explicit start
and calls [fvm_exec_duplicate_target](../include/flashvm/execution.h#L294)(context, start, source, name, depth, out).
The shared C89 service accepts values, not bytecode or an interpreter stack.
Its successful result is a native duplicate or undefined when no duplicate is
created; the Action adapter discards that result.

Depth converts to signed Int32 first, name to versioned AVM1 string second, and
source resolves third (native instances bypass string conversion). Temporary
roots retain all inputs through collecting callbacks. Absolute clone depth
subtracts [FVM_MOVIECLIP_DUPLICATE_DEPTH_BIAS](../src/standard/movieclip_constants.h#L86) before native sibling construction;
this bias is distinct from authored SWF placement and child-slot encoding.
The operation bypasses replaced duplicateMovieClip methods.

Negative or above-historical-limit clone depths create nothing, after coercion
and source resolution. Missing targets and parentless roots also create nothing.
Within Flash's historical absolute range, depths outside the currently supported
native public range return [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) rather than claim a successful clone.
The maximum historical bound is documented by its own macro. Full Flash depth
storage, non-MovieClip display objects, removed-source ancestry, unload/event
ordering, counted-NUL names and complete duplicate semantics remain open.
Native construction retains its collector barrier and post-publication error
behavior; errors preserve the C output but do not promise scene rollback.

Primary source comparison: Ruffle commit
2372a4e148a67d68efd796f4477b3cddd4f880cd, activation.rs action_clone_sprite,
globals/movie_clip.rs clone_sprite, and swf4_actions_coercion_order's original
source/output (depth.valueOf, name.toString, source.toString). These source
comparisons do not certify independent Flash Player execution equivalence.
The fetched Shumway duplicateMovieClip original is SWF15 and is not included
as SWF≤8 acceptance evidence or edited to change its version.

execution_duplicate_target is C89: host/SWF4–8, direct/path sources, aliases,
fraction truncation, Int32 wrapping, depth bias, negative/unsupported/historical
limits, missing/root targets and method override bypass.
avm1_clone_sprite covers raw bytecode, stack consumption, copied display state,
fresh dynamic properties and missing operands.
execution_duplicate_target_callbacks exercises direct C and interpreter paths,
collecting coercions, exact depth/name/source order, exceptions at each phase,
alias preservation and restored native/script/activation/root scopes.
avm1_clone_sprite_allocation injects every C and interpreter request allocation
boundary in a fresh context, allowing the documented post-publication error
semantics and collecting after each attempt.

## Русский

CloneSprite подключён к общему C-сервису явного запроса дублирования. Три операнда
потребляются без результата на стеке. Сначала глубина преобразуется в Int32,
затем имя в строку по версии SWF, затем разрешается источник. Нативные объекты
не преобразуются в строку. Входы удерживаются корнями GC при вызовах скрипта.
Публичная глубина получается вычитанием отдельного смыслового смещения.

Пропущенные операнды загруженного SWF дополняются undefined; host остаётся
строгим. Отрицательные/слишком большие глубины и отсутствующие цели дают пустой
результат. Глубины в диапазоне Flash, ещё не поддержанные моделью, явно дают
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Проверки охватывают host/SWF4–8, байткод, прямые C-вызовы,
GC, порядок преобразований, исключения и отказы памяти. Полная совместимость
сцены, типов display list и глубин остаётся открытой; все стадии открыты.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_duplicate_target` | function | [include/flashvm/execution.h:294](../include/flashvm/execution.h#L294) |
| `FVM_MOVIECLIP_DUPLICATE_DEPTH_BIAS` | macro / define | [src/standard/movieclip_constants.h:86](../src/standard/movieclip_constants.h#L86) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
