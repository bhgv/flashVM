# Local Transform assignment / Присваивание локального Transform

v0.332 implements MovieClip.transform assignment from a typed Transform retaining
another native MovieClip. The setter resolves that retained clip directly, without
reading replaceable properties of the Transform object. Plain objects, primitives
and unbound Transform values do nothing. The assigned Transform is not retained:
subsequent source changes do not move/recolor the destination. Parent, children,
name, depth and timeline identity stay with the destination.

The same operation is available through C89
[fvm_exec_clip_copy_transform](../include/flashvm/execution.h#L84)(context, destination, source). It accepts native clip
identities, with no instruction stream, interpreter stack or Action IDs. Both
operands must belong to the context and cannot be the MovieClip class prototype.
Source and destination may be identical. Matrix and color publication is atomic
on allocation/status failures; source getter side effects before publication
are retained. Temporary roots and a native GC barrier protect source queries and
the rollback transaction. A failed transaction leaves temporary color records
unreachable for ordinary later collection.

The historical callback identities remain unchanged; the new script setter adds
1402. Transport restores this setter in the destination context. No wire schema
change. Property flags now allow assignment while retaining the existing
nonenumerable/nondeletable policy.

Reference inspected: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/movie_clip.rs, set_transform. This is source comparison,
not a Flash Player oracle run. FlashVM uses its existing native local matrix
and color query semantics, including their geometry getters. Exact raw-display
snapshot behavior, setter availability/version attributes and complete Flash
Transform compatibility remain open. No roadmap stage is closed.

В v0.332 MovieClip.transform принимает другой типизированный Transform и
копирует локальные матрицу и цвет его клипа. Ссылка на источник не сохраняется;
имя, родитель, дети, глубина и кадры получателя не меняются. Обычные объекты,
примитивы и Transform без клипа игнорируются. Общий C89-сервис
[fvm_exec_clip_copy_transform](../include/flashvm/execution.h#L84) принимает два клипа, без стека и байткода;
самоприсваивание разрешено, чужой контекст и прототип класса отклоняются.

Матрица и цвет публикуются вместе. При отказах памяти оба набора полей
восстанавливаются без выделения памяти; побочные эффекты исходных getters до
начала публикации сохраняются. Корни и нативный барьер GC защищают операцию.
Новый callback 1402 переносится через существующий формат графа. Сравнение с
исходниками Ruffle не заменяет сверку с Flash Player; точность сырых снимков
сцены, исторические атрибуты и версии доступности ещё требуют работы.

Validation: Linux GCC 13.3 Release/default AGG profile passed all 652 CTest tests.
New clip_copy_transform covers host/SWF4–8, direct/script parity, reflection/shear,
RGBA offsets, self-copy, independence, child/custom-property retention, typed input,
channel restoration and getter exceptions with preserved destination side effects.
clip_copy_transform_c compiles/runs in C89. clip_copy_transform_allocation injects
all allocation boundaries for both entry paths and checks both destination snapshots
and restored root/native/script state after each error and collection.
All three new tests also passed a minimal Debug ASan heap/global + UBSan profile;
stack instrumentation and LeakSanitizer are disabled because the independent
exception probe reproduces this environment's runtime restrictions. The additional
side-effect/scene-retention assertions were rerun in Release and that sanitizer
profile after the full run. No other-platform or real Flash Player run is claimed.

Проверки: 652/652 в Linux Release/AGG; три новых теста также прошли минимальный
Debug-профиль ASan heap/global + UBSan. Стек ASan и утечки не проверялись из-за
ограничений среды, подтверждённых отдельным exception probe. Дополнительные
проверки сохранения детей и побочных эффектов getters повторно прошли оба
профиля после полного прогона. Другие платформы и Flash Player не заявляются.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_exec_clip_copy_transform` | function | [include/flashvm/execution.h:84](../include/flashvm/execution.h#L84) |
<!-- source-links:end -->
