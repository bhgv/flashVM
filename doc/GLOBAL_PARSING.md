# Global conversions and numeric prefix parsing — v0.11

`fvm_standard_install` installs native parseInt, parseFloat and String functions
alongside the earlier numeric functions. Call them by name through `fvm_call`
or AVM1 CallFunction. Installation preflights conflicting names, stages all
entries and validates these callbacks on repeated installation.

String() without arguments returns an empty string; with an argument it uses the
native ToString bridge. Extra arguments are ignored. Object conversion supports
native toString/valueOf methods and propagates pending exceptions. This addition
is a conversion function, not the complete String class or boxed String.

parseInt converts its input to a string, skips leading whitespace, reads a sign,
and accepts digits until the first invalid digit. Explicit bases support 2–36;
radix conversion uses ToNumber followed by a bounded ToInt32 conversion. Omitted,
zero, NaN or infinite radix selects an inferred base. A hexadecimal prefix selects
16; otherwise a leading zero selects 8, following the legacy AS2 documentation.
Explicit base 16 also accepts the hex prefix. No digits or an invalid base returns
NaN. A decimal point terminates the integer. Negative zero is retained.

parseFloat converts its input to a string, skips whitespace and accepts the
longest signed decimal prefix, optionally with a decimal point and exponent.
An incomplete exponent is trailing text, so `1e+tail` parses as 1. Infinity is
recognized case-sensitively. Hexadecimal and octal syntax are not inferred: a
string starting `0x` yields the decimal prefix zero. Invalid prefixes return NaN.

The whitespace set includes ASCII spacing, NBSP, Unicode space separators,
line/paragraph separators and BOM. Invalid UTF-8 is not silently skipped.
The [AS2 global function reference](https://open-flash.github.io/mirrors/as2-language-reference/global_functions.html)
provides the legacy parsing examples and octal rule.

## Verification and remaining scope

Tests cover C calls and executable AVM1 action records, default/explicit bases,
ToInt32 wrapping, all bases 2–36 using an independent integer encoder, trailing
text, incomplete exponents, Unicode leading whitespace, native conversion throws,
installation conflicts and injected allocation failures in every new function.
See tests/global_parsing.c, tests/global_parsing_avm1.cpp and error_allocation.cpp.
The C call bridge now catches allocation failure during name lookup and callback
execution and returns FVM_OUT_OF_MEMORY instead of escaping std::bad_alloc.

Decimal prefix conversion reuses the existing locale-independent numeric parser;
correct rounding for every binary64 boundary is not certified. Repeated radix
accumulation is likewise not a proof of identical Flash rounding for arbitrarily
long integers. Unicode whitespace and default-octal behavior are not yet selected
by SWF version. Complete String/Object/Function APIs, script function-valued
conversion methods and version-dependent primitive dispatch remain outstanding.

These functions are registered native callbacks, not first-class Function values.
Existing GC/channel support is unchanged. The .as example illustrates the API;
there is no ActionScript source compiler here. Builds and actual tests establish
a bounded development checkpoint, not complete SWF 8 compatibility.

Since v0.12 a native String wrapper is also available; the primitive conversion
function described here is retained. See [STANDARD_STRING.md](STANDARD_STRING.md).

## Loaded parseInt — v0.412

Loaded SWF4–8 now use a separate AVM1 path; the original host contract above
remains for unloaded contexts. No arguments return undefined. A present radix
is coerced first through loaded numeric rules and Int32 wrapping; only 2–36
are accepted, so explicit zero/undefined/NaN do not infer a base. Invalid radix
skips string conversion. Text then uses loaded ToString, once.

Raw prefixes are inspected before trimming tab/LF/CR/space. A leading unsigned
0x is stripped even with a nonhex explicit radix; a signed raw 0x is rejected
unless the explicit base accepts x (34–36), in which case its sign is ignored.
After prefix removal, whitespace and a sign may occur. Omitted radix infers
octal only for an entire raw signed/unsigned octal spelling; a trailing space,
NUL or other suffix makes it decimal. Non-ASCII bytes stop digit scanning;
embedded NUL is retained as a nondigit, not discarded before octal inference.
Negative zero and binary64 overflow are preserved. Arbitrarily long integer
rounding, other player generations and complete compatibility remain open.

Reference: pinned [Ruffle AVM1 globals](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals.rs)
and its [parseInt expected trace](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/tests/tests/swfs/avm1/parse_int/output.txt).
These are a reproducible comparison baseline, not a fresh Adobe Player run.
`global_parse_int_loaded` checks SWF4–8 prefixes, wrapping, missing values,
counted/unterminated text, callback order, invalid-radix short circuit, propagated
exceptions, native GC barriers and scope cleanup. The missing-argument assertion
failed before implementation. Existing global_parsing tests retain host rules.

Validation v0.412 / Проверка: Linux minimal Release 823/823 CTests; AGG Release
and heap/global ASan+UBSan each passed all three global-parsing executables.
Native launchers restore workspace executable modes; stack ASan/LSan disabled.
Other-platform runtime and all nine stage acceptance remain open.

## Loaded parseFloat — v0.413

Loaded SWF4–8 now use AVM1 ToString and return undefined for a missing argument.
The parsed number is accumulated digit by digit using reference-ordered decimal
scaling, rather than the host numeric conversion bridge. The pinned exact
`.1499999 == 0.14999990000000005` result is checked. Infinity spellings yield NaN.
An isolated decimal point produces zero; accumulation ignores later points,
while exponent recognition occurs only after the first grammatical fraction.
Exponent digit accumulation, sign, addition and per-digit powers wrap at 32 bits,
including INT32_MIN without signed C++ overflow. Counted NUL stops accumulation;
unterminated borrowed strings are bounded. Existing leading whitespace policy
is retained; full whitespace/player-version and extreme rounding remain open.
The unloaded host API continues to use its original prefix/Infinity behavior.

Reference: pinned [AVM1 numeric implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs)
and [parseFloat trace](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/tests/tests/swfs/avm1/parse_float/output.txt).
The loaded accumulation algorithm carries Ruffle's MIT attribution in source.
`global_parse_float_loaded` checks SWF4–8, exact rounding, decimal/exponent quirks,
NUL/surrogate/unterminated inputs, callback failure, native GC barrier and cleanup.
The missing-argument regression failed against v0.412 before implementation.
This does not certify every binary64 boundary or constitute a fresh Adobe run.

Validation v0.413 / Проверка: minimal Release 824/824; AGG Release and
heap/global ASan+UBSan each passed all four global-parsing executables.
Native mode-restoring launchers used; stack ASan/LSan disabled.
Other-platform runtime acceptance and all nine stages remain open.
