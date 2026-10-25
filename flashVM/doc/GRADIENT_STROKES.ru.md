# Градиентные обводки

Поддержанные cap/join параметры script lineStyle связаны в [SCRIPT_STROKE_STYLES.ru.md](SCRIPT_STROKE_STYLES.ru.md). Прежние файловые/нативные checkpoints ниже не подтверждают полную расширенную AS2-семантику.

Interior focal extension v0.1197+: [signed radial fill/stroke origins](FOCAL_GRADIENTS.md) / [внутренние focal-позиции](FOCAL_GRADIENTS.ru.md). Earlier zero-focal-only notes describe historical checkpoints. Endpoints/clamping, DefineShape4/LineStyle2 and all nine roadmap stages remain open.


Current color-policy extension v0.1183+: [linearRGB fills and strokes](LINEAR_RGB_GRADIENTS.md) / [интерполяция linearRGB](LINEAR_RGB_GRADIENTS.ru.md). Earlier RGB-only limits describe historical checkpoints. DefineShape4/LineStyle2 and nonzero focal remain open; all nine stages remain open.


Добавлены градиентные обводки в destination-space и ограниченная реализация Flash 8 `MovieClip.lineGradientStyle`. Поддерживаются линейные и центральные радиальные рампы, PAD, REFLECT и REPEAT с обычной RGB-интерполяцией. Все девять этапов roadmap остаются открытыми.

## Вызов C89

`fvm_path_stroke_gradient(path, surface, gradient, width, spread)` выбирает цвет из существующего [`FVM_Gradient`](../include/flashvm/gradient.h#L48) и композитит его через подготовленную маску обводки. Layout структуры не изменён; [`fvm_path_draw_gradient`](../include/flashvm/gradient.h#L66) и [`fvm_path_draw_gradient_spread`](../include/flashvm/gradient.h#L73) сохраняют семантику заливок и standalone antialiasing.

| Аргумент | Контракт |
| --- | --- |
| path | Ненулевой заимствованный векторный путь с координатами в прежних raster/path пределах. Команды не меняются и не сохраняются; открытые контуры остаются открытыми. |
| surface | Заимствованная RGBA-поверхность и её буфер с корректными размерами и stride. Вызывающий сохраняет владение и исключительный доступ на время вызова. |
| gradient | Заимствованное описание: обратимая конечная ограниченная матрица, 1–15 упорядоченных stops. Массив существует до возврата. Ограничения цветового преобразования прежние. |
| width | Конечная положительная ширина в destination-пикселях, не более [`FVM_GRADIENT_MAX_STROKE_WIDTH`](../include/flashvm/gradient.h#L10) (4096). Ноль отвергается; для hairline следует явно передать 1. |
| spread | Unsigned selector PAD=0, REFLECT=1 или REPEAT=2 из [`FVM_GradientSpread`](../include/flashvm/gradient.h#L23). Остальные значения, включая UINT_MAX, отвергаются. |

Spread применяется к отношению до интерполяции прямых RGB/alpha каналов. Затем выполняются цветовые multiply/add, покрытие маски и source-over. Caps/joins и округление наследуются от обычной обводки выбранного backend. Выборка производится в центрах destination-пикселей; побайтовое совпадение с эталонным Flash не заявляется.

Успех возвращает [FVM_OK](../include/flashvm/flashvm.h#L44). Неверные ширина, selector, геометрия, stops, матрица или поверхность дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L47) до рисования. Корректный вызов без графического backend даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). Ошибка подготовки источника или маски даёт [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L46) с сохранением пикселей. Источник и полная маска готовятся до композиции. Атомарность относится к одной обводке: предыдущие успешные paint сцены могут уже находиться в буфере при последующей ошибке.

Синхронный C89-вызов не удерживает VM, GC-корень, callback или устройство, не вызывает скрипт и не выполняет device I/O. Независимые пути, описания и поверхности можно использовать в разных потоках. Одновременная мутация заимствованных входов и запись в один буфер требуют синхронизации со стороны вызывающего.

## AS2 lineGradientStyle

Метод MovieClip установлен как nonenumerable и скрыт version-атрибутом от загруженных SWF1–7. Host-контексты без загруженного SWF его видят. Callback добавлен в конец стабильной таблицы без перенумерации прежних callback. Для восстановления графа с новым native callback нужен обновлённый получатель.

Сначала `lineStyle` включает обводку и задаёт толщину. `lineGradientStyle` сам не включает отсутствующую/выключенную обводку. Принимается прежняя модель аргументов beginGradientFill: тип, массивы цветов/alpha/ratios, копируемая box/affine матрица, optional pad/reflect/repeat, обычный RGB и нулевой focal для radial. Counted strings должны совпадать полностью. Неподдерживаемые spread/interpolation и ненулевой radial focal дают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L48). Сохраняется прежнее no-op поведение неверных массивов, матриц и типа. Отсутствие аргументов или undefined type не меняет выбранный стиль.

Цвет и alpha обводки берутся из stops; прежние color/alpha lineStyle на них не умножаются. Цветовые преобразования MovieClip и предков применяются. Толщина, fill/bitmap state и завершённые paths сохраняются. Новые path records удерживают отдельный `lineGradient`. Смена стиля создаёт новый path от текущей позиции по прежней политике сегментации. `endFill` выключает только заливку и сохраняет текущий stroke ramp. Следующий lineStyle заменяет его на solid/disabled стиль. `clear` удаляет пути, выключает обводку и освобождает текущую запись.

World matrix преобразует рампу и геометрию. Ширина наследует существующий X-axis scale policy обычных обводок; authored width=0 даёт прежний hairline в один destination-пиксель. Полная Flash-семантика caps/joins, nonuniform scale и непрерывной заливки при произвольных mid-path сменах стиля этой работой не подтверждается. Stage LOW выключает antialias покрытия; MEDIUM/HIGH/BEST используют прежний AA backend. Standalone C89-вызов всегда запрашивает AA.

## Копирование, ошибки и GC

Stops, ratios и матрица копируются; пользовательские массивы и matrix не заимствуются после выбора стиля. Ordinary RGB record использует optional spread scalar из [контракта spread](GRADIENT_SPREAD.ru.md). `lineGradient` — ребро графа, не новый resource kind или wire format. Отсутствие spread означает PAD. Нечисловые, дробные, отрицательные, NaN/infinite и выходящие за диапазон metadata отвергаются до изменения пикселей одной обводкой.

При duplicateMovieClip текущая рампа и её active-path alias сериализуются вместе. Завершённые paths удерживают прежние рампы. Binary serialization и каналы копируют граф; получатели рисуют после уничтожения отправителя. Изменение геометрии/текущего стиля дубликата не меняет источник. GC обходит обычные рёбра под native barrier; новых unmanaged roots и raster-time script callbacks нет.

Coercion аргументов может выполнять скрипт и менять state. Исключения coercion и ошибки выделения при подготовке не публикуют частичную рампу или замену active path. Явные побочные эффекты, включая reentrant изменения, не откатываются. Новая неизменяемая запись и map drawing-state готовятся до публикации. Очистка native/GC depth сохраняется.

## Примеры и проверки

[C89 tutorial](../examples/embedding/gradients/README.md) владеет путём, двумя stops и буфером. Пояснённый `stroke.inc` печатает реальные красные компоненты в фазах 0.5, 1 и 2. `check_gradient_strokes_example.py` проверяет эти значения и прежнюю заливку; CTest явно пропускается без backend. AS2 companion показывает порядок lineStyle/lineGradientStyle, но не компилируется checker в SWF.

Тесты сравнивают обводку с независимой ordinary-stroke маской, проверяют версии, копирование аргументов, reset transitions, duplicate aliases, binary/channel ownership, неверные metadata, coercion exceptions, ошибки выделения, affine/alpha/quality и независимые OS workers. Реальные AVM1 CallMethod fixtures сравнивают interpreter/manual/automatic JIT и проверяют native entry counters. Результаты сборок: [compatibility status](COMPATIBILITY_STATUS.md).

## Оставшаяся приёмка

DefineShape4/LineStyle2, linearRGB, ненулевой focal, полная lineStyle caps/joins/scaling семантика и сравнение с эталонным плеером остаются открыты. Reserved gradient bits Shape1/2/3 по-прежнему отвергаются. Аудио, дочерние потоки, A/V, перемотка, текст, остальные AS1/AS2 API, таймлайн, GC/debugger и JIT backends сохраняют независимые незавершённые задачи. Host regression/sanitizer проверки не подтверждают реальные устройства и другие платформы.

Первичный API reference: Adobe [AS2 MovieClip, lineGradientStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#lineGradientStyle()). Ограничения тегов: [SWF specification](https://open-flash.github.io/mirrors/swf-spec-19.pdf).


## v0.1182 verification

Градиентные обводки v0.1169–v0.1182: добавлены C89 [fvm_path_stroke_gradient](../include/flashvm/gradient.h#L82) и SWF8 lineGradientStyle для linear/centered-radial PAD/REFLECT/REPEAT с обычным RGB. Layout [FVM_Gradient](../include/flashvm/gradient.h#L48) и прежние callback IDs сохранены; новый callback добавлен в конец таблицы. Проверены copied current/active aliases, solid/clear/endFill transitions, duplicate и binary/channel ownership. Прошли 91 выбранный native gradient/raster/scene/graph/GC/quality/tutorial тест и дополнительный mixed fill/stroke binary-transfer fixture. Десять focused ASan/UBSan проверок и явные no-throw/no-fault запуски прошли с выключенным LeakSanitizer. Реальные AVM1-вызовы проверяют 72 сочетания host/SWF8 interpreter/manual/automatic JIT с native entry counts; четыре OS worker проходят 128 независимых paint/GC циклов. Проверки quality/affine/alpha охватывают 48 отрисовок. SDK-free C89 validation проходит; graphic tutorial явно пропускается. Переименованный detached-компонент конфигурируется, его скопированный checker проходит на native executable без заявления отдельной detached-сборки. Ошибки подготовки сохраняют прежний стиль/active path или пиксели, не отменяя побочные эффекты coercion. DefineShape4/LineStyle2, linearRGB, ненулевой focal, полная caps/joins/scaling/mid-path fill семантика, эталонный плеер, реальные устройства и другие платформы остаются открыты. Все девять этапов открыты. [Контракт](GRADIENT_STROKES.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:48](../include/flashvm/gradient.h#L48) |
| `FVM_GRADIENT_MAX_STROKE_WIDTH` | macro / define | [include/flashvm/gradient.h:10](../include/flashvm/gradient.h#L10) |
| `FVM_GradientSpread` | enum | [include/flashvm/gradient.h:23](../include/flashvm/gradient.h#L23) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:47](../include/flashvm/flashvm.h#L47) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
| `fvm_path_draw_gradient` | function | [include/flashvm/gradient.h:66](../include/flashvm/gradient.h#L66) |
| `fvm_path_draw_gradient_spread` | function | [include/flashvm/gradient.h:73](../include/flashvm/gradient.h#L73) |
| `fvm_path_stroke_gradient` | function | [include/flashvm/gradient.h:82](../include/flashvm/gradient.h#L82) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:48](../include/flashvm/flashvm.h#L48) |
<!-- source-links:end -->
