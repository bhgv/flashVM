# Cairo 1.16.0 for FlashVM

Official release: see PROVENANCE.json. Library sources, original license files
and version metadata are included. The local CMake profile builds static Cairo
with image, recording and user-font support, using bundled scalar Pixman.
X11, GL and native font discovery are disabled. FONTS enables the bundled
FreeType backend, with host-supplied faces and no Fontconfig dependency. PNG and SVG are enabled only
with IMAGE_CODECS; other document backends are disabled.

No autoconf, Meson, pkg-config, network access or preinstalled graphics library
is required. CMake generates private platform configuration headers in the build
directory; the source tree is not rewritten. Select GRAPHICS_CAIRO in DEFS.txt.
The FlashVM raster API keeps host-owned straight RGBA8 and converts through
Cairo's native-endian premultiplied ARGB32 intermediate image.

C89 changes are recorded in flashvm.patch: trailing enum commas and line comments,
64-bit constants/types, automatic aggregate assignments, container lookup without
GNU statement expressions, and one-byte trailing storage instead of a zero-sized
array. Optional unbuilt backends remain upstream sources. Native 64-bit system
types and compiler-specific inline/visibility helpers are still used by upstream.
The strict language-mode build does not imply support for every historical C89
compiler or a completed SWF display engine.

Cairo распространяется на условиях LGPL-2.1 или MPL-1.1: полные тексты лицензий
находятся в upstream/. Проверен только профиль растеризации в память на Linux.
Все изменения совместимости приведены в flashvm.patch; экранные адаптеры этим
коммитом не реализуются.

## PNG/SVG integration

IMAGE_CODECS enables bundled libpng and Cairo PNG/SVG stream APIs independently
of the FlashVM raster selection. The earlier image-only profile remains when
codecs are disabled. See [integration and limits](../../doc/IMAGE_CODECS.md).

IMAGE_CODECS включает чтение/запись PNG и запись SVG через Cairo. Подробности
и ограничения: [документация](../../doc/IMAGE_CODECS.ru.md).

Font/static integration: [English](../../doc/FONT_LIBRARIES.md), [Русский](../../doc/FONT_LIBRARIES.ru.md).
