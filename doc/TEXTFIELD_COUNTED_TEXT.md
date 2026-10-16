# Counted TextField content — v0.393

Native fields now retain a byte length with their context-owned UTF-8 buffer.
Embedded NUL participates in text/length, range validation, replaceText,
getTextFormat ranges, TextFormat.getTextExtent, native layout and contours.
Code after NUL is no longer truncated. Supplementary scalars still count as two
UTF-16 units; splitting a surrogate pair and lone-surrogate text remain unsupported.
The existing 4096-scalar/16384-byte envelope is an implementation limit, not a
Flash Player limit. Native indices remain bounded by 8192 UTF-16 units.

[GC:] A reachable field retains the entire counted arena buffer through its
existing pointer root. Replacements publish pointer/length together only after
validation and allocation. Failed edits preserve old text and style; retry works.
Snapshot size/text and restoration preserve the count. The existing graph text
payload is decoded as value text, allowing NUL with graph versions >=4; selector,
font and property-key validation is unchanged. No ABI or graph-version bump.

textfield_counted_text checks unloaded/SWF4–8 content, layout past NUL, extent,
range formatting, replacement/aliasing, traced GC and binary/queue transport with
embedded fonts. Its allocation companion sweeps setter and replaceText failures.
Font-disabled configurations exercise their existing unavailable-font policy.

Pinned source comparison: [TextField setters/replacement](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/text_field.rs)
and [EditText scalar storage](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/display_object/edit_text.rs).
This does not certify original-player NUL glyph/layout behavior. HTML, styled
spans, code-unit editing, full coercion/version rules and scene integration remain open.

## v0.394 formatting string validation

Alignment keywords now compare complete counted bytes. Font names containing
NUL reject unsupported rather than selecting a registered prefix. URL/target
accept only truly empty strings; a leading NUL no longer hides a nonempty value.
This preserves the existing supported-style subset rather than claiming URL or
new alignment/font behavior. Staged field styles remain unchanged on rejection.
textformat_counted_keywords checks current/future formatting and extent, plus
exact valid keywords and zero-count borrowed strings with nonempty backing data.

## v0.395 restored native text limits

Graph restoration now rejects native text beyond 16384 bytes or 4096 validated
Unicode scalars before allocating a Field or importing its fonts. Previously
restoration checked UTF-8 but could bypass the editing envelope. Invalid native
payloads return FVM_INVALID_ARGUMENT and graph restoration rolls back objects,
allocation counters and imported fonts. The valid 4096-scalar and supplementary
16384-byte/8192-unit boundaries remain accepted.

textfield_restore_limits grows a real serialized text payload, updates its
length/envelope size and CRC, and verifies 4097-scalar and 16385-byte rejection
with no publication or leftover state. Valid boundary records round-trip.
