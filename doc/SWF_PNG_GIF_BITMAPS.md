# PNG/GIF в SWF 8 / Embedded PNG/GIF in SWF 8

## Русский

v0.47 подключает PNG и неанимированный GIF89a в DefineBitsJPEG2/3 (21/35)
для SWF 8. Они используют общий dictionary BitmapData, bitmap-заливки,
GC и перенос сцены. PNG/GIF в более ранней SWF отклоняются; отдельные
alpha-данные в JPEG3 с PNG/GIF также отклоняются. Требуется IMAGE_CODECS;
внешние системные библиотеки не используются.

PNG декодируется статическим libpng через C89-адаптер. Перед выделением
пикселей проверяются размеры 1..2880, лимит RGBA, все границы чанков и CRC,
единственный завершающий IEND и отсутствие trailing bytes. libpng приводит
paletted/gray/RGB/RGBA, tRNS, глубины 1/2/4/8/16 и Adam7 к straight RGBA8.
При 16-bit используются старшие восемь бит; gamma correction специально
не включается. Предупреждения считаются ошибками. APNG управляющие чанки
возвращают UNSUPPORTED. C-only longjmp не пересекает C++ объекты; custom
allocator сохраняет код OOM при отказах внутренних аллокаций libpng.

GIF читается giflib потоково: не используется DGifSlurp, который мог бы
сразу выделить память для всех кадров. Проверяются canvas, лимит, image
header до вычислений giflib, палитры и индексы, GCE transparency, завершение
потока. Поддержаны глобальная/локальная палитры и interlace. Сейчас кадр
должен занимать весь canvas без смещения; второй кадр или частичный canvas
возвращают UNSUPPORTED. Прозрачные GIF-пиксели нормализуются в RGBA=0.

В обоих случаях приватный ресурс BitmapData имеет transparent=true,
включая изображения с фактически полностью непрозрачными пикселями.
PNG сохраняет входной RGB даже при alpha=0 в текущей straight-модели;
точная семантика Flash premultiplied alpha остаётся открытой. Лимит RGBA
не описывает все одновременно живущие metadata/temporary allocations.

image_codecs_swf_png_gif.cpp проверяет PNG RGBA, palette+tRNS, 2-bit gray,
16-bit gray-alpha и Adam7 RGB; GIF с глобальной/локальной палитрой и
interlace; размеры, CRC, truncation в каждой позиции, trailing data,
анимацию, версии и запрещённый alpha; настоящие пиксели сцены, GC и очередь.
image_codecs_swf_png_gif_allocation.cpp проверяет отказы C++ копирования
результата. Отказы malloc в libpng/giflib пока не инъецируются тестами.
Профили: AGG, Cairo, headless/portable с отключёнными кодеками; отдельная
сборка AGG с heap/global ASan и UBSan, без stack ASan и LeakSanitizer.

Открыто: частичный canvas GIF, полная цветовая/alpha сверка с Adobe,
metadata compatibility и recovery, DefineBits/JPEGTables, loadBitmap/draw,
фильтры, blend modes, текст и медиа. Пункт 4 ROADMAP.md не закрыт.

## English

v0.47 imports PNG and non-animated GIF89a inside SWF 8 JPEG2/3 tags through
statically bundled libpng/giflib. Earlier SWFs and separate PNG/GIF alpha
streams are rejected. Resources share the managed bitmap scene/transport
model; disabled IMAGE_CODECS reports UNSUPPORTED.

The C89 PNG adapter validates bounded dimensions, RGBA budget, chunk lengths
and CRCs, final IEND and complete input consumption. Palette/gray/RGB/RGBA,
tRNS, low-bit depths, 16-bit high-byte reduction and Adam7 become straight
RGBA8 without explicit gamma correction. Warnings are fatal; APNG control
chunks are unsupported. C-only longjmp cleanup and a custom allocator retain
allocation-specific errors without crossing C++ destructors.

GIF is streamed without an unbounded multi-frame slurp. Global/local palettes,
GCE transparency, interlace, indices and exact stream termination are checked.
Image dimensions are validated before giflib's native long multiplication.
Only one full-canvas frame is currently supported; offsets, smaller frames
and animation return UNSUPPORTED. Invisible GIF RGB is cleared.

Both formats produce transparent=true resource objects. PNG preserves hidden
RGB in current straight storage; exact Adobe premultiplied behavior remains
open. The RGBA budget does not cover all simultaneous decoder allocations.
Tests cover the named layouts, malformed streams, scenes/GC/queue transport
and failed C++ result copying. Internal library malloc failures are not yet
injected. AGG/Cairo, disabled-codec profiles and heap/global ASan+UBSan are
checked, with stack ASan and LeakSanitizer disabled in the special profile.
Legacy table JPEG, exact color/alpha conformance, remaining BitmapData APIs,
filters, blending, text and media keep roadmap stage 4 open.
