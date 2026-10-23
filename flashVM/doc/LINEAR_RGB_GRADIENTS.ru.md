# Градиенты linearRGB

Interior focal extension v0.1197+: [signed radial fill/stroke origins](FOCAL_GRADIENTS.md) / [внутренние focal-позиции](FOCAL_GRADIENTS.ru.md). Earlier zero-focal-only notes describe historical checkpoints. Endpoints/clamping, DefineShape4/LineStyle2 and all nine roadmap stages remain open.


Явная linearRGB-интерполяция добавлена для C89 gradient fill/stroke и методов Flash 8 beginGradientFill/lineGradientStyle. Layout [FVM_Gradient](../include/flashvm/gradient.h#L38), defaults старых вызовов и callback IDs сохранены. Все девять этапов roadmap открыты.

## Вызовы C89

`fvm_path_draw_gradient_interpolated(path, surface, gradient, even_odd, spread, interpolation)` выполняет заливку. `fvm_path_stroke_gradient_interpolated(path, surface, gradient, width, spread, interpolation)` рисует обводку прежним backend-покрытием. Путь, описание/stops и destination заимствуются на один синхронный вызов. VM, root, callback и устройство не сохраняются; скрипт и device I/O не вызываются.

| Параметр | Контракт |
| --- | --- |
| path | Ненулевой заимствованный путь с конечными координатами в прежних raster/path пределах. Команды не меняются. |
| surface | Ненулевая RGBA-поверхность и её буфер с корректными dimensions/stride. Вызывающий обеспечивает исключительный доступ на время вызова. |
| gradient | Прежний [FVM_Gradient](../include/flashvm/gradient.h#L38): конечная ограниченная обратимая матрица, 1–15 упорядоченных stops и прежние ограничения color transform. Layout и владение не меняются. |
| even_odd | Fill selector 0 или 1. Обводка использует прежнюю nonzero stroke mask. |
| width | Только для stroke: конечная положительная ширина в destination-пикселях, не более [FVM_GRADIENT_MAX_STROKE_WIDTH](../include/flashvm/gradient.h#L10). Ноль неверен; для одного пикселя следует явно передать 1. |
| spread | Unsigned PAD=0, REFLECT=1 или REPEAT=2, как в прежнем spread API. |
| interpolation | Unsigned [FVM_GRADIENT_INTERPOLATION_RGB](../include/flashvm/gradient.h#L28)=0 или [FVM_GRADIENT_INTERPOLATION_LINEAR_RGB](../include/flashvm/gradient.h#L29)=1. Остальные значения, включая UINT_MAX, неверны. |

Успех возвращает [FVM_OK](../include/flashvm/flashvm.h#L42). Неверные selector, width, path, surface, stops или matrix дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до рисования. Корректный запрос без backend даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Ошибка выделения samples/coverage даёт [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) и сохраняет destination bytes. Атомарность относится к одной paint-операции; более ранние успешные paints сцены не откатываются при последующей ошибке.

Старый [fvm_path_draw_gradient](../include/flashvm/gradient.h#L52) остаётся PAD/RGB. [fvm_path_draw_gradient_spread](../include/flashvm/gradient.h#L57) и [fvm_path_stroke_gradient](../include/flashvm/gradient.h#L64) сохраняют RGB для всех spread. Явный interpolation=RGB использует прежнюю byte-domain арифметику, проверенную побайтовым сравнением полных поверхностей. [fvm_gradient_init](../include/flashvm/gradient.h#L48) и public struct не получают новых полей.

## Порядок цветовых операций

RGB интерполирует encoded bytes как раньше. linearRGB нормализует RGB bytes stops до 0–1, декодирует piecewise sRGB transfer, интерполирует в линейном свете и кодирует результат обратно. Alpha всегда интерполируется как прямой линейный канал: gamma conversion и premultiply цветов stops не выполняются.

Выбран стандартный piecewise sRGB transfer, а не единая gamma-2.2 аппроксимация. Decode: c/12.92 при c<=0.04045, иначе ((c+0.055)/1.055)^2.4. Encode: 12.92*l при l<=0.0031308, иначе 1.055*l^(1/2.4)-0.055. [W3C conversion reference](https://www.w3.org/TR/css-color-4/#color-conversion-code).

Spread определяет позицию до интерполяции. Выбор duplicate stops и концов рампы прежний. Endpoints и постоянные каналы сохраняют точные byte values до inherited transforms. После обратного кодирования RGB применяются прежние byte-domain multiply/add, округление и clamp. Покрытие пути умножает alpha, затем выполняется straight-alpha source-over.

Не более пятнадцати stops декодируются один раз на paint в локальный стековый массив. Изменяемой глобальной таблицы нет. Per-pixel encoding остаётся синхронной native-математикой; нового per-pixel AVM1 metering и гарантии времени исполнения нет. Независимые пути, описания и destinations могут использовать разные потоки; общие записи и мутация входов требуют синхронизации вызывающего.

Середина чёрно-белой рампы равна 128 в RGB и 188 в данном linearRGB sampler. При alpha stops 0 и 255 alpha середины равна 128 в обоих режимах. Аналитические host-результаты не подтверждают все lookup-table/rounding детали эталонного плеера.

## ActionScript и копируемые записи

Optional седьмой аргумент beginGradientFill/lineGradientStyle принимает точные counted strings RGB и linearRGB. Отсутствующий/undefined выбирает RGB. linearRGB разрешён в загруженном SWF8 и host-контекстах без movie. Для загруженного SWF6/7 сохраняется [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) с прежним drawing state. Неизвестное написание или суффикс после встроенного NUL не принимаются. Для lineGradientStyle сохраняются SWF8 visibility и предварительный lineStyle.

Методы копируют optional numeric `interpolation` в immutable gradient record только для linearRGB. Отсутствие означает RGB, явный 0 также означает RGB. Допустимы только собственные numeric 0 и 1. Дробь, отрицательное, non-number, NaN/infinite и большие значения дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до изменения пикселей одной paint-операцией. Renderer читает raw owned metadata и не вызывает interpolation getter или inherited property.

duplicateMovieClip копирует current/active aliases в одном графе. Binary serialization и каналы сохраняют interpolation независимо от массивов/матрицы и времени жизни отправителя. Заливка и обводка удерживают разные политики. Wire/resource types и callbacks прежние. Получателю нужен новый sampler: старый reader может проигнорировать optional field и нарисовать RGB.

Coercion может выполнять скрипт. VM exceptions, native callback errors и ошибки подготовки не публикуют частичную запись или замену active path. Явные побочные эффекты, включая reentrant изменения, не отменяются. Временные объекты удерживаются обычным графом и native/GC barriers. Во время raster sampling managed callbacks не вызываются.

## Примеры и проверки

[C89 tutorial](../examples/embedding/gradients/README.md) добавляет пояснённый linear_rgb.inc. Одинаковый black/white midpoint рисуется в RGB и linearRGB для fill и stroke; печатаются реальные straight RGBA pixels. check_gradient_linear_rgb_example.py требует восемь строк, включая старую fill/spread демонстрацию. Без backend CTest явно пропускается. AS2 companion показывает SWF8 linearRGB-клип, но CMake не компилирует его в SWF.

Тесты проверяют signed linear/radial phases, тёмные transfer branches, duplicate/constant stops, alpha/source-over порядок, старую RGB byte identity, версии, counted names, copy/duplicate/binary/channel ownership, неверные metadata и отсутствие getter-вызовов, allocation/coercion ошибки, AVM1/JIT, независимые workers и stage/affine/alpha. Результаты: [compatibility status](COMPATIBILITY_STATUS.md).

## Оставшаяся работа

DefineShape4/LineStyle2 и их on-disk interpolation bits этой работой не декодируются. Reserved bits Shape1/2/3 по-прежнему отвергаются. Ненулевой focal, полная Flash stroke caps/joins/scaling и mid-path fill семантика остаются открыты. Сравнение с эталоном, производительность, реальные устройства и другие платформы требуют отдельной проверки. Аудио, дочерние потоки, A/V/seek, текст, прочие AS1/AS2/timeline, GC/debugger и остальные JIT backends сохраняют свои незавершённые задачи.

Первичный API reference: Adobe [MovieClip beginGradientFill и lineGradientStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#beginGradientFill()).


## v0.1196 verification

LinearRGB v0.1183–v0.1196: добавлены C89 fill/stroke interpolation selectors и linearRGB для beginGradientFill/lineGradientStyle SWF8, linear/centered-radial PAD/REFLECT/REPEAT. Layout [FVM_Gradient](../include/flashvm/gradient.h#L38), старые RGB defaults и callback IDs сохранены. До пятнадцати sRGB stops декодируются в paint-local память, RGB интерполируется в линейном свете и кодируется до color transform; alpha остаётся straight/linear. Endpoints и постоянные каналы точные. Прошли 105 выбранных native gradient/raster/scene/graph/GC/quality/tutorial тестов, десять focused ASan/UBSan проверок и явные no-throw/no-fault запуски; LeakSanitizer выключен. Независимые числовые таблицы проверяют тёмные transfer branches, duplicate stops, alpha/source-over и старую RGB byte identity. Проверены 72 реальных host/SWF8 AVM1 interpreter/manual/automatic JIT сочетания с native counters, четыре OS worker и 128 независимых paint/GC циклов, 96 affine/alpha/quality отрисовок. Mixed RGB/linearRGB fill/stroke binary graphs сохраняют разные политики после уничтожения отправителя. SDK-free C89 validation проходит, две graphic tutorial проверки явно пропускаются. Переименованный detached-компонент конфигурируется, скопированный checker проходит на native executable без заявления отдельной detached-сборки. Проверены counted names, версии, копирование аргументов, duplicate/current-active aliases, raw metadata без getters и ошибки подготовки. DefineShape4/LineStyle2, ненулевой focal, полная stroke scaling/caps/joins/mid-path fill семантика, per-pixel metering, эталонный плеер, устройства и другие платформы остаются открыты. Все девять этапов открыты. [Контракт](LINEAR_RGB_GRADIENTS.ru.md).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:38](../include/flashvm/gradient.h#L38) |
| `fvm_gradient_init` | function | [include/flashvm/gradient.h:48](../include/flashvm/gradient.h#L48) |
| `FVM_GRADIENT_INTERPOLATION_LINEAR_RGB` | enum constant | [include/flashvm/gradient.h:29](../include/flashvm/gradient.h#L29) |
| `FVM_GRADIENT_INTERPOLATION_RGB` | enum constant | [include/flashvm/gradient.h:28](../include/flashvm/gradient.h#L28) |
| `FVM_GRADIENT_MAX_STROKE_WIDTH` | macro / define | [include/flashvm/gradient.h:10](../include/flashvm/gradient.h#L10) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_draw_gradient` | function | [include/flashvm/gradient.h:52](../include/flashvm/gradient.h#L52) |
| `fvm_path_draw_gradient_interpolated` | function | [include/flashvm/gradient.h:71](../include/flashvm/gradient.h#L71) |
| `fvm_path_draw_gradient_spread` | function | [include/flashvm/gradient.h:57](../include/flashvm/gradient.h#L57) |
| `fvm_path_stroke_gradient` | function | [include/flashvm/gradient.h:64](../include/flashvm/gradient.h#L64) |
| `fvm_path_stroke_gradient_interpolated` | function | [include/flashvm/gradient.h:77](../include/flashvm/gradient.h#L77) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
