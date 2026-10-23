# Bitmap-обводки SWF 8 LINESTYLE2

Текущее расширение v0.1253+: [симметричные butt/square концы и bevel](STROKE_CAPS.ru.md). Прежние ограничения только round ниже исторические; miter и разные концы остаются неподдержанными.

FillType в DefineShape4 LINESTYLE2 принимает bitmap-режимы 0x40–0x43. Сохранены
repeat/продолжение края, nearest/билинейная фильтрация с premultiplied alpha,
RGBA-прозрачность, композиция матриц и качество краёв Stage. Круглые концы и
соединения, обычная масштабируемая ширина, hairline и NoClose используют те же
команды пути, что сплошные и градиентные обводки. Другие концы/соединения,
pixel hinting и non-scaling-флаги пока не поддержаны.

[fvm_swf_read_shape](../src/swf/shape.cpp#L469) проверяет стиль и владеет результатом. Загрузчик требует ранее
определённый bitmap-ресурс. Отсутствующие/нулевые ID, неверные матрицы, усечение
и старые версии SWF возвращают [FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Ошибка декодирования
сохраняет прежний результат; ошибка загрузки не публикует root.

[fvm_swf_shape_build](../src/swf/shape_graph.cpp#L117) связывает изображение обводки через lineBitmap, заливки —
через bitmap, сохраняя BitmapData из словаря. Дескриптор содержит собственные
скалярные матрицу/флаги и отслеживаемое ребро на источник, без указателя бэкенда.
Сериализация и каналы сохраняют bitmap-ресурсы и общую идентичность ссылок.
Нативный барьер защищает заимствованные пиксели; прежние ограничения потоков
контекста сохраняются. Публичный C89 ABI, номера callbacks и wire/resource-типы
не изменены.

[fvm_movieclip_bitmap_draw](../src/standard/bitmap_fill.cpp#L156) выбирает lineBitmap при положительной ширине обводки.
После dispose изображение не добавляет пикселей; живые изменения видны при
следующем рендеринге. Повреждённые собственные метаданные возвращают
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45) до записи в выходную поверхность.

Внутренний [fvm_path_bitmap_paint_quality](../src/capi/bitmap_paint_c.cpp#L155) принимает нулевую ширину для заливки или
конечную положительную ширину до [FVM_BITMAP_STROKE_MAX_WIDTH](../src/render/quality_internal.hpp#L53) для круглой обводки.
Путь, поверхности и стиль заимствуются только на время синхронного вызова;
скрипты и host callbacks не вызываются. Проверки предшествуют выбору бэкенда:
корректный вызов NO_GRAPHICS возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46), неверный —
[FVM_INVALID_ARGUMENT](../include/flashvm/flashvm.h#L45). Все отсчёты и маска подготавливаются до композиции, поэтому
источник может совпадать с выходной поверхностью. [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) сохраняет
назначение одного paint; вся сцена не является общей транзакцией. Публичный
[fvm_path_draw_bitmap](../include/flashvm/bitmap_paint.h#L26) сохраняет нулевую ширину, прежние ABI и пиксели.

## Проверки и оставшаяся приёмка

Байтовые тесты загружают настоящую сцену и сравнивают поверхности. Оракул отдельно
семплирует прямоугольник и строит сплошную маску обводки, затем объединяет покрытие
с alpha источника. Независимые центральные пиксели проверяют красный/синий цвет и
alpha для repeat/продолжения края. Покрыты четыре bitmap-режима, 20 сочетаний ширины,
16 open/closed/NoClose, 20 субпиксельных сдвигов, 16 режимов качества, удаление
отправителя, живые/disposed пиксели, неверные метаданные, aliasing источника,
проверки ID/версии, отказы памяти, перемотки, четыре OS-потока, независимые
изображения заливки/обводки и новые банки стилей.

Эталонный растр/округление Flash, точное поведение неравномерных/скошенных обводок,
остальные флаги LINESTYLE2, устройства/другие платформы и все девять этапов плана
остаются открытыми. Это ограниченное расширение файлового рендеринга, не полная
поддержка SWF8 и не новый скриптовый API рисования.

Первоисточник: Adobe SWF specification, LINESTYLE2 и bitmap FILLSTYLE:
https://open-flash.github.io/mirrors/swf-spec-19.pdf


## Итог проверки

Bitmap-обводки LINESTYLE2 v0.1239–v0.1252: DefineShape4 принимает четыре bitmap FillType для ограниченных круглых обводок, сохраняя repeat/продолжение края, nearest/билинейную фильтрацию с premultiplied alpha, живые/disposed RGBA-изображения, hairline, NoClose и качество краёв Stage. lineBitmap независим от bitmap-заливки и сохраняет идентичность BitmapData из словаря после GC, сериализации и канала. Общий внутренний семплер подготавливает отсчёты и покрытие до композиции, включая aliasing поверхностей; публичные C89 структуры/функции, callbacks и resource/wire-типы не изменены. Тринадцать новых тестов покрывают известные цвета и оракул всей поверхности, 20 сочетаний ширины/режима, 16 open/closed/NoClose, 20 сдвинутых семплингов, 16 качеств, четыре сцены через сериализацию/канал после удаления отправителя, неверные ID/версии/ширины/метаданные, live mutation/dispose, четыре серии отказов C++-памяти при декодировании/загрузке/paint, 16 перемоток, четыре OS-потока с 96 циклами загрузки/рендеринга/GC, четыре смешанные сцены с разными изображениями и четыре банка StateNewStyles. Пройдены 157 нативных тестов, включая C89-примеры bitmap-заливки и градиентов, 12 ASan/UBSan и allocation с явным --no-faults, пять SDK-free проверок без графики из новой сборки. Реальные отказы C++-памяти проверены нативно; sanitizer unwinding ограничен, LeakSanitizer отключён. Проверки ссылок и примеров проходят. Другие концы/соединения, non-scaling/hinting, точные неравномерные/скошенные обводки, эталонный растр/округление Flash и устройства/другие платформы остаются открытыми. Все девять этапов открыты. [Контракт](BITMAP_STROKES.ru.md).

[modes](../tests/swf_bitmap_stroke.cpp), [widths](../tests/swf_bitmap_stroke_widths.cpp), [close](../tests/swf_bitmap_stroke_close.cpp), [sampling](../tests/swf_bitmap_stroke_sampling.cpp), [quality](../tests/swf_bitmap_stroke_quality.cpp), [transfer](../tests/swf_bitmap_stroke_transfer.cpp), [errors](../tests/swf_bitmap_stroke_errors.cpp), [metadata](../tests/swf_bitmap_stroke_metadata.cpp), [allocation](../tests/swf_bitmap_stroke_allocation.cpp), [timeline](../tests/swf_bitmap_stroke_timeline.cpp), [threads](../tests/swf_bitmap_stroke_threads.cpp), [mixed](../tests/swf_bitmap_stroke_mixed.cpp), [styles](../tests/swf_bitmap_stroke_styles.cpp).

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_BITMAP_STROKE_MAX_WIDTH` | macro / define | [src/render/quality_internal.hpp:53](../src/render/quality_internal.hpp#L53) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_bitmap_draw` | function | [src/standard/bitmap_fill.cpp:156](../src/standard/bitmap_fill.cpp#L156) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_path_bitmap_paint_quality` | function | [src/capi/bitmap_paint_c.cpp:155](../src/capi/bitmap_paint_c.cpp#L155) |
| `fvm_path_draw_bitmap` | function | [include/flashvm/bitmap_paint.h:26](../include/flashvm/bitmap_paint.h#L26) |
| `fvm_swf_read_shape` | function | [src/swf/shape.cpp:469](../src/swf/shape.cpp#L469) |
| `fvm_swf_shape_build` | function | [src/swf/shape_graph.cpp:117](../src/swf/shape_graph.cpp#L117) |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
