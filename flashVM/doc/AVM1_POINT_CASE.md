# Point field spelling / Регистр полей точки

v0.336 adds pre-SWF7 case folding to loaded point conversion. Stored own numeric
X/Y (or mixed x/Y) are recognized in SWF4–6; SWF7–8 and unloaded host lookups
retain exact spelling. The existing pinned Flash lowercase table is reused.
Getters and inherited fields still do not participate in loaded lookup. Counted
names with an embedded NUL/suffix do not match one-unit coordinate names.

Writes preserve the selected own field spelling, avoiding a second lowercase
slot. Both names are copied before geometry callbacks can invalidate property-map
iterators. Root scopes, native barriers, ordered x/y setters and loaded input
normalization remain shared through [fvm_exec_clip_point](../include/flashvm/execution.h#L76). Native callback IDs and
wire schema do not change. Input names and counted field length use documented
macros in clip_point_constants.h.

Primary source inspected: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/object/script_object.rs get_local_stored and
core/src/avm1/globals/movie_clip.rs local_to_global/global_to_local.
This is source comparison, not Flash Player validation. Exact spelling has
priority when host APIs create duplicate case-equivalent slots; ambiguous mixed
case duplicates and geometry callbacks that replace field spellings are not
certified as Flash-equivalent. Fixed-point world matrix/output arithmetic and
raw display snapshots remain open. No roadmap stage is closed.

В v0.336 точки загруженных SWF4–6 принимают собственные числовые поля X/Y
и смешанные варианты регистра через существующую таблицу Flash lowercase.
SWF7–8 и хостовый режим сохраняют точное сопоставление. Getters, наследуемые
поля и имена со встроенным NUL/суффиксом не участвуют в таком поиске.

Запись сохраняет найденное написание поля, не создавая вторую строчную копию.
Имена копируются до геометрических callbacks, которые могут изменить карту
свойств. Общий C-сервис, корни, барьер GC, порядок setter-вызовов и нормализация
входных twips сохранены. Формат переноса и ID callbacks прежние.

Сравнение выполнено по исходникам Ruffle, без Flash Player. При созданных
хостовым API дубликатах разных регистров точное имя имеет приоритет; полное
поведение неоднозначных дубликатов и переименования полей из геометрических
callbacks не сертифицировано. Точные fixed-point матрицы и выходные twips
остаются открытыми, как и остальные стадии роудмапа.

Validation: Linux GCC 13.3 Release/default AGG passed 658/658 CTest tests.
avm1_point_case covers host/SWF4–8, both C/native entries, uppercase/mixed fields,
inverse roundtrip, unchanged spelling, inherited/virtual rejection and counted
NUL/suffix rejection. execution_clip_point_allocation now uses uppercase fields
in its SWF4–6 direct/native allocation sweeps and checks the documented write
prefix plus root/native/script restoration and subsequent collection.
Six affected tests passed minimal Debug ASan heap/global + UBSan. Stack checks
and LeakSanitizer remain disabled under the independently reproduced environment
restriction. No Flash Player, other platform or Cairo rerun is claimed here.

Проверки: 658/658 Linux Release/AGG и 6/6 затронутых тестов ограниченного
ASan heap/global + UBSan. Проверены границы версий, обе точки входа, прямое
и обратное преобразование, сохранение написания, NUL, getters и наследование.
Отказы памяти SWF4–6 теперь проверяются на uppercase-полях. Стек ASan,
утечки, Flash Player, Cairo и другие платформы в этом прогоне не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_point` | function | [include/flashvm/execution.h:76](../include/flashvm/execution.h#L76) |
<!-- source-links:end -->
