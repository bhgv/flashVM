# Font libraries checkpoint

Bundled FreeType 2.14.3, HarfBuzz 1.8.8 with recorded C++98 adaptations,
libunibreak 6.1 and FriBidi 1.0.16. HarfBuzz 10.4.0 requires newer C++;
the selected pin preserves the requested C++98 build profile.
All selected external targets are static, even with BUILD_SHARED_LIBS=ON.

Implemented owned editable vector paths, exact curve bounds, affine transforms,
glyph outlines and metrics, OpenType shaping, Unicode break classification,
bidi ordering/maps, AGG/Cairo fill/stroke rendering and image masks.
Native font handles are independent per worker. Added a C89 embedding example
and an original CC0 deterministic font fixture, with Russian/English API docs.

## Validation

Linux x86_64, GCC/G++ 13.3.0, strict C89/C++98 with pedantic errors:

| Profile | Passed programs |
| --- | ---: |
| AGG and fonts | 114 |
| Cairo and fonts | 114 |
| NO_FONTS | 110 |
| NO_GRAPHICS and NO_IMAGE_CODECS, fonts enabled | 108 |
| ASan/UBSan | 114 |

Evidence is in validation/font-libraries-*.log and *.json. The sanitizer runner
now rejects recovered diagnostics as well as failed exits. LeakSanitizer is
disabled because the execution environment restricts /proc; the historical
custom allocator exception test uses UBSan only. The instrumented core/vendor
archive was rebuilt by scripts/check.py; all programs were then rebuilt and
relinked after the empty HarfBuzz buffer fix and GC enum fix.

Checks cover copied font memory, retained outlines/runs, exact curve extrema,
composite glyphs, ligatures, kerning, Arabic forms, Hebrew clusters/bidi maps,
break classifications, masks/strokes, invalid input, empty buffers and eight
independent font workers. Static linkage was checked with archive inspection,
readelf and nm. The source manifest verifies 2094 vendored files.

The sanitizer run also found invalid enum access while validating C GC roots;
that core fix is recorded in its own Git commit. Licenses, provenance and vendor
patches are included. No mirror update was performed.

## Scope

Windows, macOS and Android execution is not verified here. These are native
primitives; complete paragraph wrapping/layout and ActionScript TextField
integration are not declared complete. Unicode datasets differ by library and
retain their documented historical coverage. See FONT_LIBRARIES.md for build,
ownership, font-size and layout limitations.
