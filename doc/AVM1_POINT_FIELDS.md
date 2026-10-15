# Loaded point fields / Поля точки в загруженном SWF

v0.334 refines fvm_exec_clip_point and the two MovieClip point methods for loaded
SWF4–8. It reads stored own numeric x/y fields without getters, inherited lookup
or numeric coercion. A primitive/missing point or an absent/nonnumeric field is
an undefined-returning no-op, before geometry callbacks. A singular inverse
uses the unchanged world point and still performs the ordered x/y writes.
Host-only contexts retain the old strict errors and getter/coercion behavior.
The C service selects the same context policy as the native script adapter.

Reference: Ruffle 2372a4e148a67d68efd796f4477b3cddd4f880cd,
core/src/avm1/globals/movie_clip.rs, local_to_global/global_to_local. This is a
source comparison, not a Flash Player oracle. Exact input/output twip arithmetic,
nonfinite-coordinate conversion and historical case-folded property spelling
remain open: this service still rejects nonfinite numeric coordinates and
uses native double matrix calculations. Matrix geometry getters retain the
existing native model. No stage is complete.

В v0.334 загруженные SWF4–8 используют собственные числовые x/y точки без
getters, наследуемых полей и преобразования типов. Отсутствующая точка, примитив
или отсутствующее/нечисловое поле дают no-op с undefined, до запросов геометрии.
Вырожденная обратная матрица сохраняет координаты точки, выполняя записи x/y.
Хостовый контекст сохраняет прежние строгие ошибки и преобразования.

Это ограниченный шаг по исходникам Ruffle, не сверка с Flash Player. Точные
вычисления в twips, неограниченные/NaN-координаты и историческое сопоставление
регистра имён ещё требуют работы: пока числовые NaN и бесконечности
отклоняются, а матрицы считаются в double. Семантика геометрических getters
не менялась; стадии роудмапа открыты.

Validation: Linux GCC 13.3 Release/default AGG passed 656/656 CTest tests,
649 initially and seven permission-related launch failures after restoring the
six affected executables. avm1_point_fields covers raw loaded SWF4–8 direct/native
calls, primitives/missing arguments, inherited fields, virtual getters, strings,
object conversion traps and suppressed geometry callbacks. execution_clip_point
checks loaded singular fallback while retaining host strictness. Allocation
injection now covers host and every SWF4–8 profile through both entry paths.
Four affected tests passed minimal Debug ASan heap/global + UBSan; stack checks
and LeakSanitizer remain disabled under the independently reproduced environment
restriction. No Flash Player, other platform or Cairo rerun is claimed here.

Проверено: 656/656 Linux Release/AGG (семь запусков потребовали восстановления
прав шести исполняемых файлов) и 4/4 изменённых тестов под ограниченным
ASan heap/global + UBSan. Отказы памяти проверены для host и всех SWF4–8.
Эталонный Flash Player, другие платформы, Cairo, стек ASan и утечки в этом
прогоне не заявляются.

## v0.335 input representation follow-up

Loaded numeric inputs now normalize NaN/Infinity and truncate/saturate to signed
32-bit twips; the v0.334 nonfinite limitation above is historical. See
AVM1_POINT_TWIPS.md. Matrix composition/application and output-twip arithmetic
remain native-double operations, not full Flash fixed-point equivalence.

В v0.335 NaN/Infinity входных числовых координат нормализуются, вход переводится
в signed 32-bit twips. Ограничение v0.334 выше историческое; точная матричная
арифметика и преобразование результата ещё открыты.
