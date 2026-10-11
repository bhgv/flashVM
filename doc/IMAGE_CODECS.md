# Bundled image libraries

`IMAGE_CODECS` is enabled by default in the root `DEFS.txt`. Select
`NO_IMAGE_CODECS` instead to omit these libraries and examples. CMake compiles
all sources from `external/`, without downloads or system-library lookup, and
publishes the required include paths and link dependencies through `flashvm`.
An embedding CMake application links its target to `flashvm`.

| Format | Bundled library | Read | Write |
| --- | --- | --- | --- |
| PNG | libpng 1.6.59 + zlib 1.3.1 | libpng; Cairo stream API | libpng; Cairo stream API |
| GIF | giflib 5.2.2 | DGif API, including multiple frames and metadata | EGif API, frames, palettes, extensions |
| JPEG | IJG libjpeg 9f | jpeg decompressor, memory source | jpeg compressor, memory destination |
| SVG | NanoSVG commit 239e102ec2c691f2902e20ace2ed36ee4a35cfe6; Cairo 1.16.0 | NanoSVG shapes and rasterization | Cairo SVG surface and stream API |

Cairo/Pixman are built for SVG export whenever IMAGE_CODECS is selected, even
with GRAPHICS_AGG or NO_GRAPHICS. The FlashVM raster backend still follows the
GRAPHICS group. NO_GRAPHICS disables that backend; the independent SVG facade
remains available with IMAGE_CODECS. With NO_IMAGE_CODECS, GRAPHICS_CAIRO builds
the earlier image/recording/user-font profile without PNG or SVG export.

Host C89 code can use `png.h`, `gif_lib.h`, `jpeglib.h`, `cairo.h`, `cairo-svg.h`
and `flashvm/svg.h`. Include stdio.h before jpeglib.h when using FILE APIs.
NanoSVG's native headers are exposed for C++98 clients; their anonymous unions
are not strict C89. Its implementation is compiled once into fvm_nanosvg; clients
must not define NANOSVG_IMPLEMENTATION/NANOSVGRAST_IMPLEMENTATION themselves.
The opaque FlashVM SVG facade is C89-compatible and owns the parsed document.
Read copies the input, size reports the viewport, rasterize writes straight RGBA8
at scale 1 with clipping and preserves row padding, and destroy releases it.

The facade bounds input to 4 MiB, viewport dimensions to 4096, transformed points
to +/-1,000,000 and total path points to 1,000,000. These checks do not make
NanoSVG a full XML validator or a hardened decoder for arbitrary hostile input.
NanoSVG supports a restricted static shape/path/paint profile, not the full SVG
standard: do not assume support for text/fonts, embedded images, filters,
scripting, external resources or complete CSS. Cairo can emit features that
NanoSVG cannot read; the tested round trip uses simple vector shapes. Set Cairo's
SVG document unit to CAIRO_SVG_UNIT_PX to preserve pixel viewport dimensions at
96 dpi. Upstream NanoSVG can silently omit unsupported content and cannot report
all allocation failures during parsing/rasterization through its APIs.

PNG uses scalar C89 sources and the upstream prebuilt configuration. GIF uses
a documented byte-sized GifBooleanType instead of C99 bool, C89 declarations and
a local reentrant tokenizer. JPEG uses its ANSI sources and memory-only backing
store; applications must install error handling (typically setjmp/longjmp) and
release jpeg_mem_dest allocations with free. NanoSVG's number parser uses double
accumulation instead of C99 strtoll, retaining fractional precision on 32-bit
long targets. All local upstream changes are recorded in flashvm.patch.
The Cairo mutex-based atomic fallback now uses int-sized status/reference storage
rather than pointer-sized access; the SVG rejected-writer test covers propagation.

Tests in tests/image_codecs_* cover memory encoding/decoding, PNG RGBA including
transparency, GIF frames/local palettes/metadata, JPEG pixels with lossy tolerance,
truncated/invalid inputs, C++98 headers, Cairo PNG and SVG stream integration,
write errors and pixel/padding checks. tests/svg_facade.c covers both enabled and
disabled builds. examples/embedding/image_codecs demonstrates SVG-to-RGBA-to-PNG
in memory and runs as a test. These are native embedding APIs, not new ActionScript
image objects or a claim that the SWF display engine is complete.

Source provenance, hashes and license notices are retained in each external
subdirectory; scripts/check_vendor_sources.py verifies all vendored files.
Cairo: LGPL-2.1-only OR MPL-1.1; Pixman: MIT-style; libpng: libpng license;
giflib: MIT; NanoSVG: zlib license; IJG: its README license and attribution.
This software is based in part on the work of the Independent JPEG Group.
See [validation](IMAGE_LIBRARIES_CHECKPOINT.md) for tested platforms and results.
