# Missing display/URL/With operands / Недостающие значения сцены, URL и With — v0.305

Loaded GetProperty/SetProperty, GetURL2 and With now prepare missing operands as
undefined before their existing semantic operations. Existing top values retain
their roles. Encoded flags/With body validation occurs before preparation.
Host-only execution retains its strict underflow policy.

| Operation with missing operands | Covered behavior |
| --- | --- |
| GetProperty with index only | Undefined target path uses existing versioned path conversion |
| SetProperty with value only | Missing target/index retain the existing versioned coercion and value-ignore rules |
| SetProperty with index/value only | The two existing values stay index then assigned value |
| GetURL2 with no values | Convert undefined URL and target; deliver through the registered host sink |
| GetURL2 with target only | Preserve that target and supply undefined URL |
| With with no value | Skip the nested body and continue after its encoded record |

The display fixture uses a real root and a child named undefined, distinguishing
empty pre-SWF7 target strings from SWF7–8's literal undefined target path. It
checks intrinsic _x reads/writes and invalid-index behavior for every missing
count in SWF4–8. Other display-property, target and historical policy limitations
are unchanged. No URL network access is performed by the fixture: its host sink
only validates the three supplied arguments synchronously.

[GC:] Original URL request roots now have documented URL/target indices and a
separate two-slot count. The old unused third root is removed; the C service
continues to retain original and converted values during callbacks. Display
operand counts/index/value offsets, encoded URL flag width and the current legacy
ASCII URL byte bound now use uppercase semantic macros in scoped headers.
The callback argument count remains a separate contract from root/operand sizes.

The With fixture embeds an observable nested assignment, verifies it is skipped
for the missing undefined value, and confirms outer execution continues with no
lexical scope left behind. Malformed With payloads and invalid GetURL2 method
flags still return [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) before semantic dispatch. Public C service
signatures and Action encodings are unchanged.

Primary comparison: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/activation.rs and runtime.rs. The audited actions use missing-pop
undefined; With skips nullish input. This is bounded source-based evidence, not
full display, URL-loading, scope or Flash Player certification.

avm1_display_url_with_missing covers host mode, SWF4–8, all missing counts,
root/child path differences, URL sink arguments and malformed records. Existing
loaded display underflow assertions are updated; raw Action bytes are unchanged.
With-scope and shared-service allocation regressions accompany sanitizer checks.

Четыре операции загруженного SWF теперь подставляют undefined в недостающие
позиции. Проверены порядок index/value, различие пустого и буквального пути
undefined, аргументы URL-синка и пропуск тела With. Повреждённые payload по-прежнему
отвергаются до выполнения. Размеры операндов и корней, индексы свойств и параметры
URL-пакета вынесены в подписанные макросы. Неиспользуемый третий URL-корень удалён.
Сетевых запросов тест не отправляет. Все стадии роудмапа остаются открытыми.

Validation: Linux AGG and Cairo each passed 588 tests, including the corrected
existing loaded URL underflow expectation. Its original failure remains in the
full-run logs. Two AGG and three Cairo launch-permission failures were repaired
and passed on retry. Four targeted heap/global ASan+UBSan fixtures passed after
explicitly rebuilding the With allocation target as well (no stack/leak coverage).
All 26 accepted original SWFs matched the pinned oracle in both profiles.
The bounded Action-reference audit remains 94/100, not semantic certification.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
<!-- source-links:end -->
