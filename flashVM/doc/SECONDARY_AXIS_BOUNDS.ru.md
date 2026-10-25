# Устойчивая Y-метрика границ обводки

Расчёт bounds теперь использует существующую `vector_length` для обоих столбцов affine-матрицы. Прежняя Y-формула sqrt(xy*xy+yy*yy) обнуляла ненулевой масштаб или теряла относительную точность при underflow квадрата нормы. X-столбец и shape hitTest уже использовали устойчивую норму. Изменяется численная устойчивость, а не выбор метрики: положительная authored width умножается на максимум длин двух столбцов; hairline остаётся одной единицей пространства назначения.

Воспроизводящий underflow-тест падает на v0.1445 до исправления. Для допустимых больших масштабов исходные координаты и authored width обратно масштабируются. Компоненты affine остаются ограничены прежним path-лимитом 1 000 000. Матрицы с показателями 510, 600 и 900 по-прежнему отклоняются до измерения обводки, даже если преобразованные точки были бы малы. Диапазоны не расширяются.

## Контракт и границы проверки

[`fvm_movieclip_set_matrix`](../include/flashvm/movieclip.h#L138) принимает конечную affine-матрицу при прежних проверках и изменяет собственное состояние clip; clip принадлежит контексту. Измерения через [`fvm_object_call`](../include/flashvm/value.h#L184) заимствуют clip и объект целевого пространства в том же контексте; `getBounds` возвращает managed-объект, который нужно удерживать GC-root при последующих VM-вызовах. Fixtures немедленно копируют четыре числа перед следующими вызовами. `getRect` сохраняет границы без обводки. Измерение, размеры и non-shape hitTest разделяют исправленную bounds-ветку. Shape-hit использовал устойчивую норму раньше и проверяется на согласованность.

Тесты используют отдельный identity-clip как целевое пространство; сингулярный source-transform допустим, но эта серия не меняет отказ при необратимом target-transform. Память, callbacks, thread/GC-ограничения публичных API сохраняются; управляемые объекты явно удерживаются и освобождаются в fixtures. Четыре worker используют отдельные runtime/контексты, не совместную мутацию одного clip.

Проверяются независимые формулы прямого отрезка, positive/negative transform, допустимые показатели -1000..17, normal/subnormal переходы и отказ для матриц вне диапазона, меняющийся X-столбец, шесть cap/join сочетаний, четыре solid/gradient paint, четыре качества, dimensions, обе формы hitTest, getRect, линейная quadratic и последовательные рёбра. Quadratic fixture редактирует native command metadata; существующий C89-пример демонстрирует реальный curveTo отдельно. 48 графов проходят binary и channel после удаления отправителя; четыре OS worker выполняют 96 bounds/GC циклов.

Пиксельный renderer по-прежнему выбирает длину X-столбца для нормальной обводки: общее согласование nonuniform/shear raster-метрики остаётся открытым. Общие non-round кривые, произвольная double-геометрия, miter/hinting/non-scaling, Cairo, полная Flash-reference и реальные платформы не закрываются. Все девять этапов плана открыты. Требуется обычная двоичная арифметика; FTZ/fast-math не проверены. Публичный C89 ABI, форматы, callback ID и кодогенерация JIT не меняются.

Существующий [C89-пример](../examples/embedding/stroke_styles/README.ru.md) остаётся исполняемым. Эта серия добавляет численные регрессии, а не пример raster с практически невидимыми координатами.


## Проверки v0.1464

Пройдены 360 нативных, 114 без графики и 114 ASan/UBSan тестов. LeakSanitizer отключён. Пройдены source-link и C89 tutorial/catalog проверки. Данные не являются проверкой физических устройств или полной Flash-reference совместимости.

| Fixture | Scope / Область |
| --- | --- |
| [movieclip_secondary_axis_underflow](../tests/movieclip_secondary_axis_underflow.cpp) | Nonzero tiny Y columns |
| [movieclip_secondary_axis_subnormal](../tests/movieclip_secondary_axis_subnormal.cpp) | Reduced-precision subnormal squares |
| [movieclip_secondary_axis_overflow](../tests/movieclip_secondary_axis_overflow.cpp) | Preserved rejection of excessive affine components |
| [movieclip_secondary_axis_normal](../tests/movieclip_secondary_axis_normal.cpp) | Ordinary admitted transforms |
| [movieclip_secondary_axis_transition](../tests/movieclip_secondary_axis_transition.cpp) | Arithmetic and admission boundaries |
| [movieclip_secondary_axis_reflection](../tests/movieclip_secondary_axis_reflection.cpp) | Reflected source geometry |
| [movieclip_secondary_axis_dominance](../tests/movieclip_secondary_axis_dominance.cpp) | Secondary-column maximum |
| [movieclip_secondary_axis_paints](../tests/movieclip_secondary_axis_paints.cpp) | Solid and gradient paints |
| [movieclip_secondary_axis_quality](../tests/movieclip_secondary_axis_quality.cpp) | Four stage qualities |
| [movieclip_secondary_axis_dimensions](../tests/movieclip_secondary_axis_dimensions.cpp) | Named width and height |
| [movieclip_secondary_axis_hitbox](../tests/movieclip_secondary_axis_hitbox.cpp) | Bounds and shape point hits |
| [movieclip_secondary_axis_rect](../tests/movieclip_secondary_axis_rect.cpp) | Stroke-free getRect |
| [movieclip_secondary_axis_curves](../tests/movieclip_secondary_axis_curves.cpp) | Axis-linear quadratic metadata |
| [movieclip_secondary_axis_contours](../tests/movieclip_secondary_axis_contours.cpp) | Consecutive segments |
| [movieclip_secondary_axis_gc](../tests/movieclip_secondary_axis_gc.cpp) | Pinned roots and collection |
| [movieclip_secondary_axis_transfer](../tests/movieclip_secondary_axis_transfer.cpp) | Binary and channel graphs after sender teardown |
| [movieclip_secondary_axis_threads](../tests/movieclip_secondary_axis_threads.cpp) | Separate runtime OS workers |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_set_matrix` | function | [include/flashvm/movieclip.h:138](../include/flashvm/movieclip.h#L138) |
| `fvm_object_call` | function | [include/flashvm/value.h:184](../include/flashvm/value.h#L184) |
<!-- source-links:end -->
