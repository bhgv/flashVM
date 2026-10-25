# Text colour ranges: required extension, not implemented API

[Editor architecture](EDITOR-CHANNELS.md) · [AS worker](as/README.md)

Inspection of `flashVM/src/standard/text.cpp`, `field_method`, shows that the setter
accepts one TextFormat object and applies a uniform style. A nonempty text range
can be queried, but there is no implemented mixed-format setter. Calling historical
`setTextFormat(begin, end, format)` currently returns UNSUPPORTED. The new AS syntax
algorithm therefore supplies spans; it does not claim those colours already appear
inside one native TextField.

The next native extension should initially support colour-only overlays, preserving
the shaping/layout of the whole text. A proposed transaction carries the expected
text revision, a bounded palette and ordered nonoverlapping ranges, and publishes
all changes only after complete validation/allocation. There are no public stub
functions or successful no-op setters in this series.

Requirements for implementation/acceptance:

- Match TextField's native UTF-16 range ABI; retain explicit conversion from Dis
  Unicode character offsets and the worker's local UTF-16 offsets.
- Reject stale revisions, invalid colour values, partial surrogate boundaries,
  overlapping/out-of-range spans and excessive run counts without changing text
  or the prior overlay.
- Apply only on the context/display owner; workers return copied span data.
- Reuse whole-run shaping positions, including kerning, ligatures, bidi and tabs.
  A colour boundary must not relayout the text as independently shaped substrings.
- Define colour selection for a glyph cluster crossing a colour boundary; do not
  silently split ligature geometry or move the caret.
- Invalidate/transform overlays explicitly on edits; do not retain obsolete ranges.
- Verify actual different pixels for different colours, uniform-style equivalence,
  scrolling/clipping, supplementary Unicode, GC/teardown and allocation failures.

Font/style changes and full historical mixed TextFormat semantics are a later,
separately tested extension. The AS presentation callback returns false when its
renderer lacks range support. This document is a concrete remaining requirement,
not evidence of the finished renderer or a new deployment dependency.
