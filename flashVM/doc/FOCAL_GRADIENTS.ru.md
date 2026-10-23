# Внутренние focal-градиенты

Для C89 и SWF8 radial gradients добавлены signed focal-позиции строго внутри нормализованного единичного круга: fill/stroke, RGB/linearRGB, PAD/REFLECT/REPEAT. Ноль сохраняет прежний centered sampler побайтно. Layout [FVM_Gradient](../include/flashvm/gradient.h#L38) и callback IDs не меняются. Все девять этапов roadmap открыты.

Это checkpoint внутренних focal. Adobe API описывает границы -1/+1 и clamping значений к ним. Для таких конечных script-аргументов здесь сохраняется [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46): endpoint policy не реализуется и не заменяется произвольной epsilon-аппроксимацией. Полная Fixed8/rounding приёмка эталонного плеера открыта.

## Вызовы C89

`fvm_path_draw_gradient_focal(path, surface, gradient, even_odd, spread, interpolation, focal)` выполняет заливку. `fvm_path_stroke_gradient_focal(path, surface, gradient, width, spread, interpolation, focal)` рисует обводку. Входы заимствуются на один синхронный вызов; VM, root, callback и устройство не сохраняются. Скрипт и device I/O не вызываются.

| Аргумент | Контракт |
| --- | --- |
| path | Ненулевой заимствованный путь с конечными координатами в прежних raster-пределах. Команды не меняются и не сохраняются. |
| surface | Ненулевая RGBA-поверхность и буфер с корректными dimensions/stride. Вызывающий обеспечивает исключительный доступ на время рисования. |
| gradient | Прежний [FVM_Gradient](../include/flashvm/gradient.h#L38): обратимая ограниченная конечная матрица, 1–15 упорядоченных stops и прежние color-transform пределы. Ненулевой focal требует [FVM_GRADIENT_RADIAL](../include/flashvm/gradient.h#L16). |
| even_odd | Fill selector 0 или 1. Stroke использует прежнее покрытие backend. |
| width | Для stroke: конечная положительная destination-ширина, не более [FVM_GRADIENT_MAX_STROKE_WIDTH](../include/flashvm/gradient.h#L10). Ноль неверен; один пиксель следует задавать явно как 1. |
| spread | Unsigned PAD=0, REFLECT=1 или REPEAT=2. |
| interpolation | Unsigned RGB=0 или LINEAR_RGB=1 из [FVM_GradientInterpolation](../include/flashvm/gradient.h#L26). |
| focal | Конечная нормализованная X-позиция с абсолютным значением строго меньше [FVM_GRADIENT_FOCAL_LIMIT](../include/flashvm/gradient.h#L9) (1). Y-позиция равна нулю. Границы и значения вне диапазона неверны. |

Успех возвращает [FVM_OK](../include/flashvm/flashvm.h#L42). Неверные geometry, focal/type, selectors, width, stops или destination дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до рисования. Корректный запрос без backend даёт [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Ошибка выделения samples/coverage даёт [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) и сохраняет пиксели. Атомарна одна paint-операция; предыдущие успешные paints сцены не отменяются при более поздней ошибке.

Прежние gradient entries используют focal zero. Старый no-selector fill остаётся PAD/RGB; spread/interpolation entries сохраняют defaults. Новые вызовы с focal=0 проверены побайтно против прежних. [fvm_gradient_init](../include/flashvm/gradient.h#L48) и public struct не получают поля.

## Геометрия и численная политика

Обратная composed matrix переводит центр destination-пикселя в normalized (x,y). Луч начинается в (f,0). Отношение равно расстоянию до выборки, делённому на расстояние от focal до переднего пересечения луча с единичной окружностью. В focal отношение ноль; на окружности — один.

Пусть dx=x-f, q=dx*dx+y*y, k=(1-f)*(1+f), h=sqrt(dx*dx+k*y*y), p=f*dx. При p>=0 отношение равно (h+p)/k. При p<0 используется равносильное q/(h-p), избегающее вычитания близких величин. Точный q=0 даёт ноль. Для f=0 прямо используется прежнее sqrt(x*x+y*y). Строгий внутренний диапазон сохраняет k положительным; endpoint epsilon/clamp не подставляется.

Spread применяется к полученному неотрицательному отношению до RGB/linearRGB interpolation. Alpha остаётся прямым линейным каналом. Порядок color transforms, path/stroke coverage и source-over прежний: [linearRGB](LINEAR_RGB_GRADIENTS.ru.md), [stroke contracts](GRADIENT_STROKES.ru.md).

Focal задаётся до gradient/world affine transforms и следует оси градиента при rotation/reflection/nonuniform transform. Stroke width сохраняет прежний X-axis scale/hairline policy. Полная Flash caps/joins/scaling и mid-path fill семантика этой работой не подтверждается. Stage LOW выключает AA краёв; MEDIUM/HIGH/BEST сохраняют прежнюю AA-политику. Standalone C89 всегда запрашивает AA.

Используются double и прежние matrix/surface пределы, без integer phase casts. Очень близкие к границе focal и большие transformed phases сохраняют обычные ограничения обусловленности/точности double; Flash Fixed8 и точные lookup-table результаты не заявляются. Нового per-pixel AVM1 metering и гарантии времени исполнения нет. Независимые входы/destinations допускают разные потоки; общие записи и мутация входов требуют синхронизации вызывающего.

## AS2 и optional copied metadata

Optional восьмой аргумент beginGradientFill/lineGradientStyle приводится к числу только для radial. Отсутствующий/undefined или zero сохраняет центр. Ненулевой внутренний focal требует загруженного SWF8; host-контексты его принимают. В более раннем loaded profile сохраняется [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Linear игнорирует аргумент без вызова coercion callback. Visibility lineGradientStyle и предварительный lineStyle прежние.

Конечные boundary/outside значения дают [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) без публикации стиля. Nonfinite radial coercion сохраняет прежний успешный no-op. Script-side clamping к endpoint-градиентам пока отсутствует. Coercion может вызывать valueOf и менять state. VM/native errors и ошибки подготовки сохраняют прежний published style/active path, если сам скрипт их не меняет; явные эффекты и reentrant изменения не отменяются.

Immutable descriptor хранит optional own numeric `focal` только для ненулевого положения. Отсутствие или явный ноль означает centered. Restored non-number, nonfinite, boundary/outside или ненулевой focal с linear geometry дают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до изменения пикселей одной paint-операцией. Renderer читает raw owned scalar и не вызывает focal getter или inherited property.

Stops, matrix и focal копируются. duplicateMovieClip клонирует current/active aliases в одном графе. Binary serialization и каналы сохраняют разные fill/stroke origins и цветовые политики после уничтожения отправителя. Resource kind, wire format и callback ID не добавляются. Получателю нужен новый renderer: старый reader может проигнорировать optional scalar и нарисовать centered geometry.

Временные managed objects удерживаются обычным графом и прежними native/GC barriers. Ray sampler имеет только локальное числовое состояние, без unmanaged roots, callbacks, shared caches и async work.

## Примеры и проверки

[C89 tutorial](../examples/embedding/gradients/README.md) добавляет пояснённый focal.inc. Измеряются пиксели signed interior и centered рамп на общем принадлежащем main прямоугольнике. check_gradient_focal_example.py требует четырнадцать строк, включая прежние spread/linearRGB результаты; без backend CTest явно пропускается. AS2 companion показывает противоположные focal для fill/stroke, но CMake не компилирует его в SWF.

Тесты проверяют горизонтальные лучи, off-axis контуры, оба знака, near-boundary interior, точный focal/centered, selectors и неверные metadata, отсутствие getter, copy/duplicate/binary/channel ownership, mixed origins, coercion/allocation ошибки, реальные AVM1 interpreter/JIT, независимые OS workers и affine/alpha/quality. Результаты сборок: [compatibility status](COMPATIBILITY_STATUS.md).

## Оставшаяся приёмка

Focal endpoints/clamping, Flash Fixed8, сравнение округления/производительности с эталоном открыты. DefineShape4/LineStyle2 и on-disk focal structures не декодируются; reserved gradient bits Shape1/2/3 по-прежнему отвергаются. Реальные устройства и другие платформы учитываются отдельно. Аудио/дочерние потоки, A/V/seek, текст, прочие AS1/AS2/timeline, GC/debugger и другие JIT backends сохраняют свои незавершённые задачи.

Первичный API reference: Adobe [MovieClip beginGradientFill и lineGradientStyle](https://open-flash.github.io/mirrors/as2-language-reference/MovieClip.html#beginGradientFill()).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_Gradient` | struct | [include/flashvm/gradient.h:38](../include/flashvm/gradient.h#L38) |
| `FVM_GRADIENT_FOCAL_LIMIT` | macro / define | [include/flashvm/gradient.h:9](../include/flashvm/gradient.h#L9) |
| `fvm_gradient_init` | function | [include/flashvm/gradient.h:48](../include/flashvm/gradient.h#L48) |
| `FVM_GRADIENT_MAX_STROKE_WIDTH` | macro / define | [include/flashvm/gradient.h:10](../include/flashvm/gradient.h#L10) |
| `FVM_GRADIENT_RADIAL` | enum constant | [include/flashvm/gradient.h:16](../include/flashvm/gradient.h#L16) |
| `FVM_GradientInterpolation` | enum | [include/flashvm/gradient.h:26](../include/flashvm/gradient.h#L26) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_draw_gradient_focal` | function | [include/flashvm/gradient.h:85](../include/flashvm/gradient.h#L85) |
| `fvm_path_stroke_gradient_focal` | function | [include/flashvm/gradient.h:90](../include/flashvm/gradient.h#L90) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
