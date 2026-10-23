# Устойчивая Y-метрика границ обводки

Расчёт bounds теперь использует существующую `vector_length` для обоих столбцов affine-матрицы. Прежняя Y-формула sqrt(xy*xy+yy*yy) обнуляла ненулевой масштаб при underflow или давала бесконечность при overflow, хотя сама длина была конечной. X-столбец и shape hitTest уже использовали устойчивую норму. Изменяется численная устойчивость, а не выбор метрики: положительная authored width умножается на максимум длин двух столбцов; hairline остаётся одной единицей пространства назначения.

Воспроизводящий underflow-тест падает на v0.1445 до исправления. Для больших масштабов исходные координаты и authored width обратно масштабируются, чтобы преобразованная геометрия и эффективная ширина оставались в прежних допустимых диапазонах. Новых разрешённых диапазонов нет.

## Контракт и границы проверки

[`fvm_movieclip_set_matrix`](../include/flashvm/movieclip.h#L107) принимает конечную affine-матрицу при прежних проверках и изменяет собственное состояние clip; clip принадлежит контексту. Измерения через [`fvm_object_call`](../include/flashvm/value.h#L126) заимствуют clip и объект целевого пространства в том же контексте; `getBounds` возвращает managed-объект, который нужно удерживать GC-root при последующих VM-вызовах. Fixtures немедленно копируют четыре числа перед следующими вызовами. `getRect` сохраняет границы без обводки. Измерение, размеры и non-shape hitTest разделяют исправленную bounds-ветку. Shape-hit использовал устойчивую норму раньше и проверяется на согласованность.

Тесты используют отдельный identity-clip как целевое пространство; сингулярный source-transform допустим, но эта серия не меняет отказ при необратимом target-transform. Память, callbacks, thread/GC-ограничения публичных API сохраняются; управляемые объекты явно удерживаются и освобождаются в fixtures. Четыре worker используют отдельные runtime/контексты, не совместную мутацию одного clip.

Проверяются независимые формулы прямого отрезка, positive/negative transform, показатели -1000..900, normal/subnormal/overflow переходы, меняющийся X-столбец, шесть cap/join сочетаний, четыре solid/gradient paint, четыре качества, dimensions, обе формы hitTest, getRect, линейная quadratic и последовательные рёбра. Quadratic fixture редактирует native command metadata; существующий C89-пример демонстрирует реальный curveTo отдельно. 48 графов проходят binary и channel после удаления отправителя; четыре OS worker выполняют 96 bounds/GC циклов.

Пиксельный renderer по-прежнему выбирает длину X-столбца для нормальной обводки: общее согласование nonuniform/shear raster-метрики остаётся открытым. Общие non-round кривые, произвольная double-геометрия, miter/hinting/non-scaling, Cairo, полная Flash-reference и реальные платформы не закрываются. Все девять этапов плана открыты. Требуется обычная двоичная арифметика; FTZ/fast-math не проверены. Публичный C89 ABI, форматы, callback ID и кодогенерация JIT не меняются.

Существующий [C89-пример](../examples/embedding/stroke_styles/README.ru.md) остаётся исполняемым. Эта серия добавляет численные регрессии, а не пример raster с практически невидимыми координатами.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_movieclip_set_matrix` | function | [include/flashvm/movieclip.h:107](../include/flashvm/movieclip.h#L107) |
| `fvm_object_call` | function | [include/flashvm/value.h:126](../include/flashvm/value.h#L126) |
<!-- source-links:end -->
