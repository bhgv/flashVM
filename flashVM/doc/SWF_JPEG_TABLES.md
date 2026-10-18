# Общие JPEG tables / Shared SWF JPEG tables

## Русский

v0.48 добавляет JPEGTables (8) и DefineBits (6) для SWF 1..8. Таблица
определяется один раз в root dictionary до использующих её изображений.
Таблицы внутри DefineSprite и повторный JPEGTables отклоняются.

Tables/Misc stream должен иметь SOI и завершающий EOI. Проверяются framing,
длины маркеров и отсутствие trailing bytes; разрешены DQT, DHT, DAC, DRI,
APPn и COM, остальные маркеры возвращают UNSUPPORTED. Содержимое таблиц
дополнительно проверяется libjpeg при декодировании использующего их bitmap.
Формат отдельных неиспользуемых DQT/DHT payload не проверяется целиком.
Исторический лишний префикс FF D9 FF D8 разрешён только до SWF 8.

При DefineBits удаляются EOI таблицы и SOI изображения, затем создаётся
приватный полный JPEG stream. Его декодирует существующий bounded C89
libjpeg bridge. Результат входит в общую модель BitmapData, static placement,
bitmap fills, GC и сериализации; параметры размеров/цветов/recovery/лимита
те же, что SWF_JPEG_BITMAPS.md. NO_IMAGE_CODECS возвращает UNSUPPORTED для
использующего таблицы изображения. Ошибки не публикуют частичный bitmap.

image_codecs_swf_jpeg_tables.cpp создаёт настоящий JPEG, разделяет DQT/DHT
и abbreviated image, проверяет truncation таблиц и изображения, marker
framing, старый префикс, missing/duplicate tables, пиксели размещённого
bitmap во всех SWF версиях 1..8, GC и перенос через очередь.
image_codecs_swf_jpeg_tables_allocation.cpp инъецирует отказы new/new[]
нормализации таблиц, склейки/копирования и полной загрузки сцены.
Проверки выполнены в AGG/Cairo и специальном heap/global ASan+UBSan профиле;
headless/portable сохраняют lossless и disabled-codec проверки.

Открыто: другие JPEG markers/исторические recovery-варианты, таблицы после
первого использующего DefineBits, CMYK и точная Adobe color/alpha семантика.
Завершение графического подпункта или этапа 4 не заявляется: остаются
BitmapData APIs, masks/filters/blending, полноценный текст, звук и видео.

## English

v0.48 imports a single root JPEGTables stream and table-based DefineBits
resources for SWF 1..8. Tables must precede images; duplicates and dictionary
resources inside sprites are rejected. SOI/EOI, marker lengths and exact
framing are validated. DQT/DHT/DAC/DRI/APPn/COM are accepted; other markers
are unsupported. Referenced table contents are further validated by libjpeg;
unused DQT/DHT payloads are not completely interpreted.

Legacy prefixes are accepted only before SWF 8. The importer removes the
joining EOI/SOI pair, builds one private complete JPEG and invokes the bounded
C89 bridge. Resources reuse existing scene/fill/GC/transport behavior and
JPEG dimension/color/recovery limitations. Disabled codecs return UNSUPPORTED.
Failed decoding preserves output and never exposes partial resources.

Tests split an actual encoded JPEG, cover table/image truncation, prefixes,
missing/duplicate tables, placement pixels across versions 1..8, GC/queue
transport and injected C++ allocation failures. AGG/Cairo and the special
heap/global ASan+UBSan profile are checked; disabled-codec profiles retain
lossless coverage. Other marker/recovery variants, late tables, CMYK and
exact Adobe color behavior remain open, alongside remaining graphics APIs,
text and media. Stage 4 is not complete.
