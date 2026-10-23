# Режимы продолжения градиента

Current stroke extension v0.1169+: [lineGradientStyle and C89 strokes](GRADIENT_STROKES.md) / [градиентные обводки](GRADIENT_STROKES.ru.md). Earlier lineGradientStyle limitations below describe the historical checkpoint; current RGB/zero-focal stroke support does not close full Flash line semantics or any roadmap stage.


Новый C89-вызов [`fvm_path_draw_gradient_spread`](../include/flashvm/gradient.h#L50) принимает unsigned selector: [`FVM_GRADIENT_SPREAD_PAD`](../include/flashvm/gradient.h#L20) (0), [`FVM_GRADIENT_SPREAD_REFLECT`](../include/flashvm/gradient.h#L21) (1), [`FVM_GRADIENT_SPREAD_REPEAT`](../include/flashvm/gradient.h#L22) (2). Layout [`FVM_Gradient`](../include/flashvm/gradient.h#L31) не изменён. Старый вызов по-прежнему выбирает PAD.

Для нормализованного отношения r режим repeat использует r-floor(r), полуоткрытый интервал. Reflect приводит r по модулю двух, нормализует отрицательную фазу и разворачивает второй интервал. PAD сохраняет прежнее поведение концов и совпадающих stops. Политика применяется к линейным и центральным радиальным градиентам перед интерполяцией straight RGBA в обычном RGB, цветовым преобразованием, маской и композицией.

Данные заимствуются на один синхронный вызов; контекст, GC-корни и устройство не сохраняются. Неверные selector, stops, матрица и поверхность дают INVALID_ARGUMENT до рисования. Корректный вызов в NO_GRAPHICS даёт UNSUPPORTED. Ошибка выделения памяти при подготовке сохраняет пиксели. Ограничения поверхности и affine остаются прежними.

`MovieClip.beginGradientFill` принимает counted strings pad, reflect и repeat. Непериодические ограничения прежние; reflect/repeat разрешены для загруженного SWF 8 и host-контекстов. Для более раннего загруженного SWF возвращается UNSUPPORTED. Суффикс после встроенного NUL не игнорируется.

Неизменяемая запись градиента хранит optional numeric `spread` только для режимов, отличных от PAD. Отсутствие поля означает PAD. Копирование, duplicateMovieClip, binary serialization и channel transfer сохраняют scalar и aliases записей. Получателю нужен обновлённый renderer. Приведение аргументов может вызывать побочные эффекты скрипта: ошибка сохраняет прежний градиент и active path, но не отменяет эти эффекты. При sampling скрипт не вызывается.

## Проверки и незавершённая работа

Тесты проверяют аналитические пиксели linear/radial, отрицательные фазы, швы, периодические сдвиги, побайтовое совпадение старого PAD, версии SWF, counted names, копирование аргументов, duplicateMovieClip, GC, binary/channel transfer после уничтожения отправителя, ошибки выделения памяти и исключения coercion. Это host-проверки, а не доказательство полной совместимости с эталонным Flash.

DefineShape4, linearRGB, ненулевой focal gradient и lineGradientStyle остаются открыты. Правила DefineShape1/2/3 не изменены. Все девять этапов roadmap открыты. Реальные устройства и другие платформы проверяются отдельно.


Gradient spread v0.1160–v0.1168: добавлены unsigned C89 selectors PAD/REFLECT/REPEAT без изменения layout [FVM_Gradient](../include/flashvm/gradient.h#L31) и старого PAD-вызова, режимы beginGradientFill для Flash 8 и optional metadata копируемых записей. Прошли 16 выбранных native gradient/quality/example тестов, три focused ASan/UBSan теста (LeakSanitizer выключен) и C89 validation сборки NO_GRAPHICS. Ошибки подготовки сохраняют прежний стиль и пиксели; ошибки coercion сохраняют явные побочные эффекты скрипта. Выполнение новых режимов через JIT, конкурентность workers, сравнение с эталонным плеером, реальные устройства и другие платформы этой серией не подтверждены. DefineShape4, linearRGB, ненулевой focal и lineGradientStyle открыты; все девять этапов roadmap также открыты. [Контракт](GRADIENT_SPREAD.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:31](../include/flashvm/gradient.h#L31) |
| `FVM_GRADIENT_SPREAD_PAD` | enum constant | [include/flashvm/gradient.h:20](../include/flashvm/gradient.h#L20) |
| `FVM_GRADIENT_SPREAD_REFLECT` | enum constant | [include/flashvm/gradient.h:21](../include/flashvm/gradient.h#L21) |
| `FVM_GRADIENT_SPREAD_REPEAT` | enum constant | [include/flashvm/gradient.h:22](../include/flashvm/gradient.h#L22) |
| `fvm_path_draw_gradient_spread` | function | [include/flashvm/gradient.h:50](../include/flashvm/gradient.h#L50) |
<!-- source-links:end -->
