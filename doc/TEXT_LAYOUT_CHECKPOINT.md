# Text layout checkpoint

Implemented owned single-font horizontal layout in flashvm/text_layout.h.
The implementation resolves paragraph bidi levels, separates script/level runs,
shapes logical segments with line context, wraps measured candidates and reshapes
final lines. It preserves byte clusters, whitespace, empty hard lines and CRLF.
Metrics separate advances from exact ink bounds. Alignment, baseline spacing,
block/glyph vector export and font-independent ownership are included.

## Acceptance evidence

Linux x86_64, GCC/G++ 13.3.0, strict C89/C++98 with pedantic-errors,
CMake 3.31.10. Bundled static targets remain selected with BUILD_SHARED_LIBS=ON.

| Profile | Passed test/example programs |
| --- | ---: |
| AGG | 117 |
| Cairo | 117 |
| NO_FONTS | 113 |
| NO_GRAPHICS and NO_IMAGE_CODECS | 111 |
| Full scripts/check.py --sanitize rebuild | 117 |

Evidence: validation/text-layout-*.log and *.json. The sanitizer runner rebuilt
all core/vendor objects and every program; recovered diagnostics fail the run.
LeakSanitizer is disabled because /proc is unavailable. The historical custom
allocator exception test uses UBSan only. No new sanitizer diagnostics occurred.

The new tests cover ligatures/kerning, grapheme-safe emergency wraps, empty lines
and CRLF, Hebrew/Latin/Arabic mixing, line-boundary Arabic joining, directional
overrides/isolates, exact curved glyph bounds, owned editable copies, invalid
UTF-8/options/enums and bounded shaping work. An additional 128 generated mixed
strings under AUTO/LTR/RTL yield 384 comparisons with FriBidi visual maps.
Eight workers exercise independent layout construction and simultaneous reads /
editable exports of a shared layout whose font has already been freed.
The C89 embedding example runs in all four profiles. The vendor source manifest
still verifies 2094 files; no external dependency source changed in this step.

## Remaining work

This checkpoint does not complete SWF TextField/TextFormat. The layout is one
font/style, horizontal, with simple script inheritance. Font fallback, styled
spans, caret/hit testing, full script itemization, tabs, discretionary hyphens,
vertical text and justification remain explicit limits. Work limits and greedy
wrapping policy are documented in TEXT_LAYOUT.md. Windows/macOS/Android builds
and execution remain unverified. No mirror update was performed.
