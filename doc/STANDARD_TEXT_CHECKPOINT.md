# Standard text checkpoint

Added named native constructors and rooted own/prototype accessor callbacks.
AVM1 NewObject calls only the constructor registry, preserving argument order
and validating same-context object results. Invalid C value enum representations
are now validated by bytes before C++ scalar access. Constructor and accessor
captures participate in GC; callback replacement drops previous captures.
Native accessor resources cannot be silently lost through graph transfer.

Added opt-in TextFormat construction/getTextExtent and a host-created TextField
bridge backed by the copied per-context font registry and owned text layout.
Current/future whole-field formatting is independent, and field outlines survive
GC/context destruction. Known unsupported display features fail explicitly.
See STANDARD_TEXT.md and the C89 standard_text embedding example.

## Verification

Linux x86_64, GCC/G++ 13.3.0, strict C89/C++98, pedantic-errors,
CMake 3.31.10 with BUILD_SHARED_LIBS=ON:

| Profile | Passed programs |
| --- | ---: |
| AGG | 121 |
| Cairo | 121 |
| NO_FONTS | 117 |
| NO_GRAPHICS and NO_IMAGE_CODECS | 115 |
| ASan/UBSan | 121 |

Logs/reports are in validation/standard-text-*.log and *.json. The sanitizer
archive was rebuilt from current core/vendor sources by scripts/check.py;
a subsequently updated value_c object was rebuilt, then every program was
rebuilt and relinked against the final archive. Recovered diagnostics are
rejected. The final native_properties test was also rebuilt/rerun after adding
automatic AVM1 collection pressure. LeakSanitizer remains disabled because /proc
is unavailable; the historical custom allocator test uses UBSan only.

Tests cover AVM1 construction/method/property calls, argument order/underflow,
constructor result errors, distinction from ordinary native functions, prototype
accessor receivers, read-only/write-only behavior, rooted captures/replacement,
automatic and explicit GC, invalid C enum values, copied input font data,
formatted extent/wrapping/negative leading, current/future snapshots, ignored
read-only writes, UTF-16 length, invalid UTF-8 rollback, unsupported features,
field resource collection and independent contours. Static bundled font symbols
are defined in the embedding executable; runtime dependencies are system-only.
The unchanged vendor manifest verifies 2094 files.

## Mirror

On the explicit command, master was updated from 969ae9c to 2cdedd6, including
font libraries and native layout. Remote HEAD/master hashes and a fresh bare
clone (92 commits, git fsck --full) verified the update. HTTP/2 initially failed;
the successful send used HTTP/1.1 with buffered request data. The attempted
refs/stash publication was rejected by automatic review and was omitted from
the successful master-only update. Stash remains in the authoritative local
repository/archive. This checkpoint's subsequent development was NOT pushed.
The internal Git/source archive remains primary; updates require direct commands.

## Remaining scope

This is a Flash 6–8 text API subset with native external fonts, not complete
SWF TextField/TextFormat compatibility. MovieClip/display-list creation,
SWF font/text decoders, styled ranges, HTML, selection/caret/input, scrolling,
font fallback, all format attributes and version-specific coercion remain
outstanding. Registries are per context and not implicitly copied to workers.
Windows/macOS/Android execution is not verified. Detailed current policy and
limits are explicit in STANDARD_TEXT.md and TEXT_LAYOUT.md.
