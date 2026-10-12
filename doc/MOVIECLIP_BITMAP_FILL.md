# Bitmap-заливки / Bitmap fills

MovieClip.beginBitmapFill принимает BitmapData, необязательную матрицу,
repeat (по умолчанию true) и smoothing (по умолчанию false). Матрица a/b/c/d/tx/ty
копируется при вызове; BitmapData удерживается живой ссылкой. setPixel/setPixel32
меняют последующую отрисовку всех использующих его путей. endFill/clear и новый
beginFill/beginGradientFill снимают только текущий bitmap-стиль; завершённые
пути удерживают собственные дескрипторы до удаления геометрии.

repeat=true повторяет изображение. repeat=false продолжает крайние пиксели,
а не делает область за изображением прозрачной. Nearest sampling берёт pixel
centers; smoothing использует билинейную фильтрацию с предварительным умножением
цвета на alpha, затем возвращает straight RGBA. Поэтому невидимый цвет
прозрачного соседнего пикселя не создаёт цветной ореол. Матрица заливки
компонуется с world-матрицей клипа; RGBA ColorTransform родителей применяется
после выборки, перед source-over через антиалиасную маску пути AGG/Cairo.

C89 API bitmap_paint.h задаёт источник, матрицу, флаги и RGBA multiply/add.
Источник может совпадать с назначением: временный sampled image полностью
читается до записи. Размеры/stride/матрицы/пути проверяются до отрисовки.
В NO_GRAPHICS возвращается UNSUPPORTED, но managed drawing state сохраняется.
Сингулярная/невалидная матрица или BitmapData отклоняются до изменения стиля.
Dispose удерживаемого источника прекращает его отрисовку, сохраняя геометрию.

[GC:] Дескриптор и каждый использующий его путь удерживают BitmapData.
DuplicateMovieClip копирует контуры и active-path alias, но сохраняет исходную
ссылку на текстуру: изменения BitmapData видны обоим клипам. При перенесении
в другой контекст граф получает независимый pixel buffer, сохраняя общие
ссылки внутри одной посылки. Проверены queue/file и loopback TCP без установки
стандартных глобальных объектов в принимающем контексте.

При отказе памяти публикация нового стиля атомарна. Временные vector paths
освобождаются при обычном выходе и исключениях в render/bounds/shape-hit.
Выявлены и исправлены узкие exception-safety дефекты в AGG: публикация cell
counts, освобождение пустых pointer tables и замена pod_array/pod_vector.
Исходники внешней библиотеки не переформатированы; исправления перечислены
в external/agg/README.md. Регрессия проверяется всеми границами new/new[] в
movieclip_bitmap_allocation, включая баланс C++-выделений после уничтожения VM.
Это не полная проверка C-heap внешних библиотек и не замена LeakSanitizer.

Тесты: bitmap_paint.c, movieclip_bitmap_fill.cpp, movieclip_bitmap_allocation.cpp;
учебный пример examples/embedding/bitmap_fill/main.c. Пиксельные проверки идут
в AGG и Cairo; headless/portable проверяют доступность API и managed state.
Ограничения: историческое fixed-point/premultiplied округление Flash, качество
при уменьшении, attachBitmap, SWF bitmap-теги, bitmap-aware hitTest, draw,
фильтры, текст и медиа ещё не завершены. Этап 4 остаётся открытым.

## English

beginBitmapFill retains a live BitmapData and copies its matrix/flags. Tiling,
edge extension, nearest sampling and premultiplied-alpha bilinear filtering feed
the existing antialiased AGG/Cairo path mask and inherited color transforms.
The C89 bitmap painter supports aliased source/destination surfaces by sampling
before writes. Current styles publish atomically on OOM; completed paths retain
their descriptors. Duplication copies geometry while sharing texture identity;
graph transport creates independent receiver pixels and preserves packet aliases.
Queue/file/loopback TCP, GC, pixels, style transitions and allocation failures are
covered by the named tests and the commented C tutorial. Targeted AGG allocation
exception fixes and RAII native-path cleanup prevent corruption/leaks detected
by the new failure-injection suite. Upstream code was not reformatted.

Full Flash rounding, quality/downsampling, attachment, resource tags, bitmap hit
testing, drawing, filters, text and media acceptance remain open. Stage 4 is not
complete. Leak/stack sanitizer limits and external C-heap coverage remain explicit.
