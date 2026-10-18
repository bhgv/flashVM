# TextField range editing checkpoint

The host-created TextField bridge now supports replaceText and character/range
getTextFormat reads through C callbacks and AVM1 CallMethod. End indices are
exclusive and agree with the existing UTF-16 length property. Uniform current
format survives edits to nonempty fields; insertion into an empty field adopts
future format. This is an explicit interim policy, not full Flash formatting parity.
See STANDARD_TEXT.md for exact supported behavior and remaining restrictions.

Validation precedes mutation. Invalid UTF-8, invalid indices, surrogate-splitting
boundaries and exceeded budgets leave text/style/layout unchanged. Output remains
limited to 4096 Unicode scalars / 16384 UTF-8 bytes. UTF-16 length can reach 8192.
All allocations are owned by the context or temporary RAII containers. Previous
text snapshots survive edits and collection; exported paths stay independent.
Successful edits discard cached layout so widths, heights and contours refresh.

## Verification

Linux x86_64, GCC/G++ 13.3.0, CMake 3.31.10, strict C89/C++98 with
pedantic-errors. Existing bundled dependencies remain static.

| Profile | Successful tests/examples |
| --- | ---: |
| AGG | 122 |
| Cairo | 122 |
| NO_FONTS | 118 |
| NO_GRAPHICS and NO_IMAGE_CODECS | 116 |
| ASan/UBSan (AGG) | 122 |

The sanitizer runner rebuilt every selected core/vendor source, then compiled,
linked and executed all programs against that final instrumented archive.
Recovered sanitizer diagnostics also fail the runner. LeakSanitizer is disabled
because /proc is unavailable; the historical custom allocator exception test
uses UBSan only. Both the final editing test and updated embedding example were
compiled after their final source edits. The unchanged vendor manifest verifies
2094 bundled source files.

tests/standard_textfield_editing.c covers native and AVM1 edits, every valid
range of a five-character ASCII string, deletion/insertion/end boundaries,
Cyrillic and supplementary Unicode, invalid/reversed/fractional/infinite indices,
surrogate splitting, invalid UTF-8 rollback, current/future styles, format reads,
cached wrapped heights, aliased replacement strings, scalar/byte ceilings,
old snapshots after GC, and independent paths after later edits. Font operations
run in enabled profiles. The C89 embedding example actually performs edits and
format reads. editing.as is illustrative source, not an externally compiled corpus.

Reports and logs are in validation/text-editing-*.json and *.log. One initial
CTest invocation could not execute an existing test because its execute bit was
missing in the workspace. Restoring executable permissions and rerunning all
programs produced the successful profile results above.

## Remaining scope and mirror

Styled range writes, HTML/CSS, selection/replaceSel, display-list/timeline
integration, SWF embedded fonts and full coercion/version semantics remain
unimplemented. The UTF-8 string model cannot store isolated UTF-16 surrogates;
editing inside a pair explicitly returns UNSUPPORTED. No Windows/macOS/Android
execution or full TextField/SWF 8 compatibility is claimed.

The internal Git/archive is authoritative. This development checkpoint is not
pushed: the mirror remains at the explicitly requested 0a9c53d update. Future
updates require a direct command and the documented 64 MiB HTTP-buffer workflow.


v0.393: counted embedded-NUL storage/edit/layout/transport added; see
[EN](TEXTFIELD_COUNTED_TEXT.md) / [RU](TEXTFIELD_COUNTED_TEXT.ru.md).
