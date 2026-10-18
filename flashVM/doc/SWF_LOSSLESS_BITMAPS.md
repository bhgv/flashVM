# Lossless bitmap-ресурсы SWF / SWF lossless bitmap resources

## Русский

В v0.45 загрузчик принимает DefineBitsLossless (20, SWF 2+) и
DefineBitsLossless2 (36, SWF 3+). Статически подключённый bundled zlib
распаковывает данные; выбранные GRAPHICS_AGG/GRAPHICS_CAIRO используют
одну модель RGBA и bitmap-заливок. NO_GRAPHICS сохраняет ресурсы и граф
сцены, но возвращает [FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46) при растеризации.

Поддержаны палитры RGB/RGBA, строки индексов с padding до четырёх байтов,
big-endian RGB555 с padding и XRGB/ARGB. Входные premultiplied-компоненты
переводятся в текущую straight RGBA модель BitmapData: округление
`(component * 255 + alpha / 2) / alpha`, ограничение 255, RGB=0 при alpha=0.
Это определённое поведение реализации, а не заявление о полной числовой
эквивалентности Adobe Flash.

Проверяются ID, размеры, формат, индексы палитры, зарезервированные биты,
длина распакованных строк и точное потребление zlib-потока. Повреждённые,
обрезанные и дополненные потоки не публикуют частичный результат. Размеры
ограничены 2880×2880 текущей моделью BitmapData; больший ресурс даёт
[FVM_UNSUPPORTED](../include/flashvm/flashvm.h#L46). Совокупный размер декодированных RGBA ресурсов и размер
отдельного распакованного буфера ограничены лимитом загрузчика. Этот лимит
не является полной оценкой одновременных временных аллокаций VM.

DefineShape/2/3 принимают bitmap-заливки 0x40–0x43: повтор или продолжение
краевого пикселя, smoothing или nearest. Варианты без smoothing требуют
SWF 8. Линейная часть bitmap MATRIX переводится из twips в пиксели; её
перенос уже нормализован общим SWF reader. Ресурс должен быть определён
раньше использующей его фигуры; повторные ID и ссылки на отсутствующий
bitmap отклоняются. Dictionary-ресурсы внутри DefineSprite отклоняются.

Один ресурс имеет один управляемый BitmapData, общий для всех его заливок.
Прямое размещение bitmap ID создаёт скрытый статический прямоугольный узел.
Это покрытая тестом реализация; полная сверка прямого размещения с Flash
остаётся открытой. Восстановленная сцена доступна рендереру в контексте без standard_install.
Создание новой сцены через load_swf требует установленного MovieClip.
Внутренняя фабрика пикселей без установленного класса не добавляет script-методы.
Граф ресурсов, общие ссылки и пиксели сохраняются при передаче в другой
контекст через очередь, файл и loopback TCP; GC удерживает их через сцену.

Тесты: swf_lossless_bitmap.cpp (форматы, alpha, padding, stream boundaries,
лимит и неизменность результата при ошибке), swf_bitmap_fill.cpp (четыре
режима, единичная матрица, реальные пиксели, dictionary aliases, GC и
каналы), swf_bitmap_allocation.cpp (отказы new/new[] декодирования,
построения сцены и рендеринга). Проверены headless, AGG, Cairo и portable
профили; специальные sanitizer проверки описываются отдельным отчётом.

Открыто: JPEG/JPEG2/JPEG3, BitmapData.loadBitmap/draw, полный набор bitmap
операций, эталонная квантизация alpha, совместимость нестандартного RGB555
в Lossless2, Shape4/LineStyle2, фильтры, маски и blend modes. Пункт 4
ROADMAP.md не завершён.

## English

v0.45 imports DefineBitsLossless (tag 20, SWF 2+) and DefineBitsLossless2
(tag 36, SWF 3+) using statically bundled zlib. Indexed RGB/RGBA palettes,
four-byte row alignment, big-endian RGB555 and XRGB/ARGB are decoded.
Premultiplied input becomes current straight RGBA BitmapData storage with
rounded `(component * 255 + alpha / 2) / alpha`, clamping and zero RGB for
zero alpha. Exact Adobe rounding equivalence remains unverified.

Validation covers nonzero IDs/dimensions, supported formats, palette indices,
reserved bits, decoded length and complete consumption of a single zlib
stream. Failed decoding preserves the previous output. Dimensions above
2880 are unsupported. Aggregate decoded pixel bytes and each decompressed
buffer obey the importer limit; complete peak-memory accounting is separate.

Shape1/2/3 bitmap styles 0x40–0x43 share one native BitmapData per resource.
Repeat/PAD and smooth/nearest modes use normalized pixel-space matrices;
non-smoothed styles require SWF 8. Missing/duplicate IDs and dictionary
resources inside sprites are rejected. Direct bitmap placement creates a
hidden rectangular static node, with full reference conformance still open.
Restored scenes render without installing script classes in the receiver;
loading a new SWF requires an installed MovieClip class.
Queue, file and loopback TCP graph transport preserve resource aliases, and
scene edges retain native pixels through GC. NO_GRAPHICS supports decoding
and transport but returns UNSUPPORTED for rendering.

The three named tests cover binary formats and malformed streams, rendered
fills, resource identity, GC/transport and injected allocation failures.
Headless, AGG, Cairo and portable profiles are checked; sanitizer evidence
belongs in the separate validation report. JPEG tags, loadBitmap/draw,
remaining BitmapData operations, exact alpha quantization, unusual Lossless2
RGB555 compatibility, filters/masks/blending and the rest of stage 4 remain
open.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_UNSUPPORTED` | enum constant | [include/flashvm/flashvm.h:46](../include/flashvm/flashvm.h#L46) |
<!-- source-links:end -->
