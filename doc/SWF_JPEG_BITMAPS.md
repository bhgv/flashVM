# JPEG bitmap-ресурсы SWF / SWF JPEG bitmap resources

## Русский

v0.46 подключает DefineBitsJPEG2 (21, SWF 2+) и JPEG3 (35, SWF 3+) к
общему dictionary BitmapData и существующим bitmap-заливкам. Используется
статически включённый IJG libjpeg 9f из external/; IMAGE_CODECS обязателен.
NO_IMAGE_CODECS возвращает FVM_UNSUPPORTED при загрузке JPEG, сохраняя
возможность загрузки lossless-ресурсов через zlib.

Поддержаны RGB, YCbCr и grayscale JPEG, включая преобразование grayscale
в RGB. Сначала читается заголовок, затем проверяются размеры 1..2880 и
лимит RGBA; только после этого запускается декодирование пикселей. CMYK
и другие пространства пока возвращают UNSUPPORTED. Предупреждения libjpeg
считаются ошибками: повреждённый или обрезанный JPEG не принимается через
автоматическое восстановление. Остаток после EOI тоже отклоняется.
Исторический лишний префикс FF D9 FF D8 перед SOI принимается только до SWF 8.
Другие исторические варианты склейки пока не реализованы.

JPEG3 строго проверяет AlphaDataOffset, единственный zlib-поток и ровно
width×height байтов alpha. RGB считается premultiplied, переводится в
текущую straight RGBA модель с тем же округлением, что lossless-данные;
значения сверх alpha ограничиваются при переводе, при alpha=0 RGB=0.
Точная цветовая/числовая эквивалентность Adobe не подтверждена.

C89-адаптер src/capi/swf_jpeg_c.c удерживает состояние декодера, временную
строку и результат в heap, освобождая всё при longjmp внутри C. Longjmp
не проходит через C++ stack/destructors. В C++ пиксели копируются в
контейнер проекта с отдельной обработкой bad_alloc; частичный результат
не публикуется. Лимит RGBA не является полной квотой временной памяти
libjpeg или всех одновременных буферов.

image_codecs_swf_jpeg.cpp проверяет реальные JPEG/JPEG3, grayscale,
обрезание в каждой позиции, trailing bytes, alpha, размеры/лимиты, версии,
рендеринг, GC и очередь в контекст без standard_install.
image_codecs_swf_jpeg_allocation.cpp проверяет отказы new/new[] копирования
пикселей и загрузки сцены; это не инъекция отказов внутреннего malloc libjpeg.
swf_jpeg_disabled.cpp проверяет поведение профиля без кодеков. Проверяются
AGG/Cairo, headless/portable и отдельная инструментированная сборка AGG
с ASan/UBSan; результаты и ограничения инструментов выдаются отдельно.

Открыто: DefineBits/JPEGTables, PNG/GIF внутри JPEG2/3, CMYK, более широкая
совместимость JPEG recovery, loadBitmap/draw и остальные операции
BitmapData. Пункт 4 ROADMAP.md остаётся открытым.

## English

v0.46 imports DefineBitsJPEG2 and JPEG3 into the shared bitmap resource graph
using statically bundled IJG libjpeg 9f. IMAGE_CODECS is required; the disabled
profile reports UNSUPPORTED while retaining zlib lossless decoding.
RGB/YCbCr and grayscale are supported, with 1..2880 dimensions and an RGBA
budget checked before pixel decompression. CMYK remains unsupported.
Warnings, truncated streams and bytes after EOI are rejected. The historical
FF D9 FF D8 prefix is accepted only before SWF 8.

JPEG3 validates the image offset and exact zlib alpha length/consumption.
Premultiplied RGB becomes current straight RGBA storage using the lossless
rounding rule, clamping channels and clearing invisible RGB. Exact Adobe
color/rounding equivalence remains unverified.

The C89 bridge confines libjpeg longjmp to C, retaining all changed state in
heap storage and cleaning decoder pools, rows and unpublished output.
The C++ boundary catches failed copies without publishing partial resources.
The RGBA limit does not account for all decoder/temporary peak allocations.
Tests cover binary failures, rendered pixels, GC/queue transport and injected
C++ allocation failures; they do not inject libjpeg malloc failures. Disabled
codecs and instrumented AGG builds are also checked, with technical evidence
kept outside Git. Legacy table-based JPEG, embedded PNG/GIF, CMYK, recovery
compatibility and the remaining stage 4 features are still open.
