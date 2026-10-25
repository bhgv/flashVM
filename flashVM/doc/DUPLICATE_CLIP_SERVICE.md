# Intrinsic sibling construction / Общее дублирование клипа — v0.321

[fvm_exec_duplicate_clip](../include/flashvm/execution.h#L408)(context, clip, name, depth, initial, out) is a C89
value-based interface to existing native sibling construction. The name and
public depth are explicit values; initial may be NULL or an initialization
value. It neither reads AVM1 instructions nor looks up duplicateMovieClip.
The stable script callback now adapts to this service, preserving callback IDs
used by graph transport. The intrinsic bridge calls the existing construction
routine directly. Root instances have no sibling and produce undefined.

Inputs are validated for context ownership and retained before callbacks.
The existing construction collector barrier remains active; nested collection
is blocked. Result publication is delayed until success and may alias any input.
Errors preserve the caller's output. This does not promise scene rollback:
instance initialization or constructor errors can occur after child publication.
The existing native depth range, name coercion, counted-NUL rejection, display
and drawing copying, registered classes, frame setup and initialization policy
are unchanged. Full Flash duplicate semantics, versioned return rules, removed
source ancestry and the complete Flash depth range remain open.

execution_duplicate_clip exercises direct C drawing/display copying, independent
geometry, initialization, same-depth replacement, root rejection and channel/file
transport. Its file path differs from the script fixture for parallel CTest.
execution_duplicate_clip_versions covers host/SWF4–8, aliases, invalid operands
and direct execution after overriding the script method.
execution_duplicate_clip_allocation enumerates construction allocation failures
with a fresh context per boundary, checking unchanged aliased output, restored
root/native/script scopes and collection after every attempt.

## Русский

Общий C-интерфейс дублирования принимает явные значения имени, публичной глубины
и необязательной инициализации. Скриптовый метод использует этот же сервис;
идентификаторы нативных callbacks для сериализации сохранены. Сервис не зависит
от инструкций, стека интерпретатора или подменяемого duplicateMovieClip.

Входы проверяются и удерживаются корнями GC; при создании действует прежний
нативный барьер сборщика. Выход публикуется только при успехе и может совпадать
со входом. Откат сцены после ошибок конструктора не заявляется. Существующие
ограничения глубины, имени и копирования сохранены. Проверки охватывают прямые
C-вызовы, host/SWF4–8, графику, транспорт и отказы памяти. Полная совместимость
дублирования и завершение стадий роудмапа не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_duplicate_clip` | function | [include/flashvm/execution.h:408](../include/flashvm/execution.h#L408) |
<!-- source-links:end -->
