# Image library checkpoint — 2026-10-11

Added offline static libpng 1.6.59, giflib 5.2.2, IJG JPEG 9f and pinned NanoSVG
sources, public include/link integration, IMAGE_CODECS/NO_IMAGE_CODECS selection,
Cairo PNG/SVG support, a C89 SVG facade and an in-memory SVG → RGBA → PNG example.
Upstream licenses/provenance/patches and the source hash manifest are retained.
A Cairo LP64 atomic-fallback bug discovered by the rejected SVG writer test was
fixed; the original 1590-byte glyph-name table was preserved exactly during its
C89 initializer conversion. See [API and limitations](IMAGE_CODECS.md).

Validation on Linux x86_64, GCC/G++ 13.3.0, CMake 3.31.10:

| Configuration | Tests/examples passed |
| --- | --- |
| AGG + IMAGE_CODECS | 108/108 |
| Cairo + IMAGE_CODECS | 108/108 |
| NO_GRAPHICS + IMAGE_CODECS | 108/108 |
| AGG + NO_IMAGE_CODECS | 102/102 |
| Cairo + NO_IMAGE_CODECS | 102/102 |
| Standalone strict C89/C++98, ASan/UBSan | 108/108 |

The sanitizer run excludes AddressSanitizer for the historical custom-new/delete
container exception test (UBSan remains); LeakSanitizer is disabled because the
managed environment cannot expose /proc task information. No leak validation is
claimed. All builds use strict language modes and pedantic errors. 23 real CMake
configuration cases and 1044 bundled-file SHA-256 checks passed. Log/JSON evidence
is in doc/validation/image-libraries-*. Initial CTest permission errors caused by
lost execute bits in this environment were resolved and complete suites rerun.

Reproduce with CMake configure/build/ctest, selecting DEFS.txt keys or an absolute
FVM_DEFS_FILE copy. Run scripts/check_definitions.py, scripts/check_vendor_sources.py
and scripts/check.py --sanitize for the additional checks. Builds perform no
library downloads or system-library discovery.

Windows/macOS/Android execution and cross-architecture builds were not available
for validation. This checkpoint does not claim full SVG support, new ActionScript
image objects, complete Flash/SWF support or JIT completion. The Git mirror was
not updated. The full source/Git archive retains the previous unfinished-work
stash without applying it to this dependency checkpoint.
