# External dependency sources

`external/` is the single dependency directory. Existing mini_regex was moved
from `extern/`; no forwarding directory remains. Build paths and live documentation
were updated. Historical validation reports retain their original paths.

| Dependency | Version | Integration |
| --- | --- | --- |
| [minimp3](../external/minimp3/README.md) | ea99364f, unchanged CC0 | Static scalar Layer-III core, even NO_MEDIA |
| mini_regex | Existing byte-oriented subset | Always built |
| zlib | 1.3.1 | Bounded CWS decompression |
| AGG | 2.4 core, pinned fork commit | RGBA8 polygon rasterizer |
| Cairo | 1.16.0 | Alternative polygon rasterizer, image/recording/user-font profile |
| Pixman | 0.38.4 | Scalar compositing dependency of Cairo |
| SQLite | 3.50.4 | Bundled static C adapter, enabled by default |

All library source files needed by the selected profiles are included. There
are no configure-time downloads or system-library fallbacks. Each vendor keeps
upstream license notices; new pins include official archive URLs, SHA-256 hashes,
and complete local source patches. `python3 scripts/check_vendor_sources.py`
checks the vendored source manifest offline.

C89/C++98 language modes remain strict, with pedantic errors on GCC/Clang and
errors for undeclared C functions. Library implementation helpers still depend
on native 64-bit system types and supported compiler inline/visibility facilities.
These are not a promise of compatibility with every historical ANSI compiler.
Cairo 1.18.4's upstream C11 build was not chosen; 1.16.0's selected core is compiled
and tested with explicit compatibility patches. Pins are reproducibility choices,
not assertions that these are the newest releases or a complete upstream audit.

IMAGE_CODECS adds PNG/GIF/JPEG reading and writing and restricted SVG reading
(NanoSVG) / writing (Cairo). See [image libraries](IMAGE_CODECS.md).
Cairo GUI, GL, native fonts and PDF/PS remain disabled; Pixman
assembler/SIMD is disabled. SQLite dynamic extension loading is disabled. FlashVM
system adapters stay in `ports/`; upstream library platform internals remain in
`external/`. No third-party source was reformatted wholesale. Windows, macOS
and Android execution remains unvalidated. See [build definitions](BUILD_DEFINITIONS.md)
and [checkpoint validation](DEPENDENCY_CHECKPOINT.md).

FONTS adds bundled static FreeType 2.14.3, HarfBuzz 1.8.8, libunibreak 6.1 and
FriBidi 1.0.16, plus AGG/Cairo FreeType adapters. Fontconfig/system font discovery
remains disabled. [Profile and compatibility limits](FONT_LIBRARIES.md).
