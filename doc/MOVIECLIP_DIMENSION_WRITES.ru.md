# Индексная запись размеров — v0.385

SetProperty 8/9 и общий C-сервис теперь изменяют масштаб векторного MovieClip.
Сохраняется порядок преобразований index→target→value; null/undefined и numeric
NaN не меняют размер. Ошибки/исключения распространяются. Выбранный клип остаётся
корнем, даже если valueOf отсоединяет его и выполняет настоящий GC. Именованные
setters _width/_height этим изменением не устанавливаются.

Непреобразованные границы корневых контуров включают локальные матрицы потомков
и прежнюю модель толщины линий. Собственная матрица клипа и матрицы предков
исключены: обратная матрица не нужна, прежний нулевой масштаб и вырожденный
родитель не мешают операции. Прежние intrinsic scales и rotation читаются после
обхода границ. Формула закреплённого reference для поворота вычисляет оба масштаба;
результирующие ширина/высота могут отличаться от простого присваивания requested
размера. Отрицательные запросы, отражения, Infinity и вырожденная геометрия
используют эту формулу; нечисловой/бесконечный вычисленный масштаб становится нулём.
Нулевые знаменатели безопасно приводят к тому же окончательному fallback.

[GC:] Обход использует снимки узлов и нативный барьер. Getter преобразований
потомка может выполняться; собственное преобразование корня для этих границ не
читается. Оба scale готовятся в копии свойств и публикуются одним swap без
выделения. OOM/ошибка сохраняют значения масштабов перед публикацией; прежние
побочные эффекты callbacks не откатываются. Выход записывается только при успехе,
в том числе поверх входа. Корни и нативная глубина восстанавливаются.

Конечные масштабы, не помещающиеся в прежнее процентное представление,
возвращают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) до публикации. Authored shear сохраняет свой basis,
но точные Flash skew/rounding не подтверждаются. Открыты named intrinsics,
точные twip/stroke/cap/join границы, masks/filter/text/bitmap extents, смешанные
player-профили и полная геометрия. Сохраняется ограниченная double vector модель.
ABI, callback IDs и wire не меняются.

Проверки: movieclip_dimension_write — host/SWF4–8, empty/flat/ordinary геометрия,
отрицательные размеры, константы повёрнутых масштабов, потомки, вырожденные предки,
NaN/Infinity/null/undefined и переполнение представления; allocation sweep обоих
измерений — атомарность scales/output, GC/retry; callbacks — настоящий GC при
отсоединении, исключения/ошибки/alias, сохранение побочных эффектов geometry getter
и обход root-transform/named-width setter; jit_dimension_write — SetProperty и
чтение в интерпретаторе, явном и автоматическом JIT для SWF4–8.

Сопоставление: [формулы размеров](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object.rs)
и [преобразование свойства](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/stage_object.rs)
закреплённого Ruffle. Нового прогона оригинального Flash Player нет.
Все девять стадий остаются открытыми.

Validation v0.385: all 807 AGG / 807 Cairo / 788 minimal Linux Release checks
have passing evidence after six ELF execution-permission repairs and targeted
reruns (one AGG, two Cairo, three minimal). Eight restricted heap/global
ASan+UBSan checks pass: four new dimension write/allocation/callback/native
cases plus previous dimension reads, AVM1 display properties, native indexed
reads and geometry allocation. Stack ASan and LeakSanitizer remain disabled.
The revised MovieClip constants header passes a strict C89 syntax probe.
No new original external SWF oracle or foreign-OS run is claimed.
All nine roadmap stages remain open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
