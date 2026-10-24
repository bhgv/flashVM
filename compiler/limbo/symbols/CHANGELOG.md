# Source-map implementation changes / Изменения

- 0.1.1: Add bounded CocoR C99 reader for Limbo sbl source-map prefix.
- 0.1.2: Cover symbol source-map basic.
- 0.1.3: Cover symbol source-map inherited.
- 0.1.4: Cover symbol source-map files.
- 0.1.5: Cover symbol source-map crlf.
- 0.1.6: Cover symbol source-map empty.
- 0.1.7: Cover symbol source-map opaque.
- 0.1.8: Cover symbol source-map versions.
- 0.1.9: Cover symbol source-map cuts.
- 0.1.10: Cover symbol source-map counts.
- 0.1.11: Cover symbol source-map coordinates.
- 0.1.12: Cover symbol source-map lengths.
- 0.1.13: Cover symbol source-map nul.
- 0.1.14: Cover symbol source-map random.
- 0.1.15: Cover symbol source-map extent.
- 0.1.16: Cover symbol source-map spellings.
- 0.1.17: Cover symbol source-map zeros.
- 0.1.18: Cover symbol source-map line endings.
- 0.1.19: Check source-map decoding against the upstream Limbo symbol writer.
- 0.1.20: Record source-map generator, sanitizer and upstream interoperability acceptance.

## 0.1.21–0.1.24

Immutable coordinate, bounded forward-line breakpoint and statement-ID queries;
checked native sblquery inspector; C output-preservation/edge tests and 240
independent randomized CLI oracle queries. See README for the precise policies
and the boundary between static source queries and runtime Dis stepping.
