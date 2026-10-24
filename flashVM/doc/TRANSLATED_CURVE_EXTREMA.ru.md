# Экстремумы перенесённых кривых Bezier

## Ошибка и численная политика

Малая кривая рядом с большой координатой теряла значащие разряды производной при вычитании взвешенных абсолютных controls. Для origin 524288, единицы 2^-33 и локальных cubic controls [14,53,1,23] прежняя верхняя граница равнялась 27 единицам; независимый локальный oracle даёт примерно 28,995047089778474. Исправленный представимый результат равен 29.

Общий helper `curve_bounds` сначала учитывает конечную точку. Если начало ненулевое, все активные controls имеют тот же знак и их модули лежат между половиной и удвоенным модулем начала, лемма Стербенца обеспечивает точное вычитание локальных координат при обычной двоичной IEEE-арифметике с постепенным underflow. Производные и вычисление Bernstein используют локальные координаты; origin прибавляется один раз к каждому экстремуму. Другие конфигурации сохраняют прежнюю арифметику. Для малых discriminants сохраняется прежнее масштабирование двоичным показателем.

Изменение действует в публичном [`fvm_path_bounds`](../include/flashvm/path.h#L71) и private round/round stroke bounds. Выделения памяти и сохраняемого состояния нет. Публичные заголовки, ownership, статусы, callback identities и wire formats не меняются. Cubic остаётся native path; команды cubic MovieClip и генерация JIT не добавляются.

## Контракт API и пример

[`fvm_path_bounds`](../include/flashvm/path.h#L71) заимствует неизменяемый ненулевой path и записывает четыре собственных скаляра в ненулевой output. Null возвращает [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45); допустимое измерение, включая пустой нулевой box, возвращает [FVM_OK](../include/flashvm/flashvm.h#L42). Владение не передаётся, VM, GC и host callbacks не вызываются. Независимые paths можно читать параллельно; изменение того же path во время измерения требует синхронизации вызывающей стороны. Admission координат и прежняя обработка move-only contours сохраняются. Выделение и редактирование path по-прежнему могут завершаться ошибками собственных API.

Существующий исполняемый [пример C89 cubic](../examples/embedding/cubic_extrema/README.ru.md) остаётся применимым и повторно проверяется во всех выбранных профилях. Новых публичных функций нет.

## Ограничения

Это условный расчёт в локальных координатах, а не доказательство интервального охвата и не полный общий решатель корней. Квантование координат, почти кратные корни, далеко разнесённые controls разных знаков, произвольные non-round curves и согласование raster/transform metrics остаются отдельными задачами. Физические устройства, Cairo, FTZ/fast-math и недвоичные платформы не проверены. Все девять этапов roadmap открыты.

## Регрессионные случаи

Семнадцать fixtures сравнивают нормализованный результат с независимым long-double oracle производной на малых целочисленных controls до переноса. Допуск 0,51 единицы разрешает округление восстановленной абсолютной координаты и отвергает прежнюю ошибку почти в две единицы. Матрица меняет восемь форм, три оси, знаки и двоичные origins. Другие fixtures отдельно проверяют quadratic, linear, repeated-root, оба cubic humps, round strokes, native affine transforms и малые origins до 2^-900. Повторно запускаются прежние tiny-cubic и stroke регрессии.


## Контрольная точка v0.1504

Восстановленное изменение проходит 149 выбранных исполняемых проверок no-graphics Release и 149 no-graphics ASan/UBSan Debug в новых каталогах сборки. Оба набора включают все семнадцать новых fixtures и существующий пример C89 cubic. Все процессы завершаются нулём; assertions включены. LeakSanitizer отключён. Runtime выводит предупреждения поиска имени executable (errno 2), что может ограничивать символизацию; sanitizer violation не зарегистрирован, все тестовые процессы завершаются нулём. До обслуживания workspace тот же исходный код также прошёл 395 выбранных native AGG/JIT и 149 minimal проверок; завершения были подтверждены, но native-сборка удалена обслуживанием. Прерванный прежний text-JIT sanitizer запуск не учитывается.

Публичные заголовки не меняются. Серия из двадцати коммитов применяется через git am --3way к чистому worktree от опубликованной базы v0.1484; проверяются совпадение итоговых деревьев, последовательные версии и обе подписи bhgv. Документация со ссылками на исходники и frontend tutorial checker проходят. Generated logs остаются вне Git. Ни один этап roadmap не закрывается.


| Fixture / Проверка | Source / Исходник |
| --- | --- |
| analytic | [analytic](../tests/path_translated_curve_analytic.cpp) |
| horizontal | [horizontal](../tests/path_translated_curve_horizontal.cpp) |
| diagonal | [diagonal](../tests/path_translated_curve_diagonal.cpp) |
| negative | [negative](../tests/path_translated_curve_negative.cpp) |
| origins | [origins](../tests/path_translated_curve_origins.cpp) |
| tiny | [tiny](../tests/path_translated_curve_tiny.cpp) |
| quadratic | [quadratic](../tests/path_translated_curve_quadratic.cpp) |
| hump | [hump](../tests/path_translated_curve_hump.cpp) |
| linear | [linear](../tests/path_translated_curve_linear.cpp) |
| repeated | [repeated](../tests/path_translated_curve_repeated.cpp) |
| round | [round](../tests/path_translated_curve_round.cpp) |
| round negative | [round_negative](../tests/path_translated_curve_round_negative.cpp) |
| round tiny | [round_tiny](../tests/path_translated_curve_round_tiny.cpp) |
| transform | [transform](../tests/path_translated_curve_transform.cpp) |
| transform tiny | [transform_tiny](../tests/path_translated_curve_transform_tiny.cpp) |
| transform round | [transform_round](../tests/path_translated_curve_transform_round.cpp) |
| matrix | [matrix](../tests/path_translated_curve_matrix.cpp) |

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `fvm_path_bounds` | function | [include/flashvm/path.h:71](../include/flashvm/path.h#L71) |
<!-- source-links:end -->
