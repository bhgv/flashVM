# Global conversions and numeric prefix parsing — v0.11

[`fvm_standard_install`](../include/flashvm/standard.h#L20) installs native parseInt, parseFloat and String functions
alongside the earlier numeric functions. Call them by name through [`fvm_call`](../include/flashvm/value.h#L138)
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
execution and returns [FVM_OUT_OF_MEMORY](../include/flashvm/flashvm.h#L44) instead of escaping std::bad_alloc.

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

## Loaded traversal budgets — v0.414

Loaded parseInt/parseFloat share the current AVM1 work scope, including radix
and ToString callbacks; nested calls cannot reset the parent's budget. Integer
octal inspection, consumed ASCII spaces and inspected digit candidates are
charged. Float leading whitespace scalars, consumed grammar bytes and inspected
accumulation bytes are charged. Decimal scaling has at most 32 squaring steps
per digit. The quota is FlashVM's execution policy, not a claim about Adobe's
instruction accounting. Missing arguments do no scanning; unloaded host rules
remain unbudgeted. General input validation/coercion work is not fully charged.

`global_parsing_loaded_work` checks exact thresholds, 65537-digit input, nested
conversion, inherited/saturated counters, retry and host isolation. Its first
quota assertion failed on v0.413. `global_parsing_loaded_allocation` sweeps every
allocation up to success through callback conversion, then collects and checks
scope cleanup. `jit_global_parsing` checks SWF4–8 interpreted/attached/automatic
native CallFunction: exact reference value, radix rule, missing arguments,
shared work counts, bounded failure and retry; real native entries are asserted.
Other JIT architectures, worker-stop stress and full conformance remain open.

Validation v0.414 / Проверка: minimal Release 827/827; AGG Release 7/7 and
heap/global ASan+UBSan 7/7 parsing/JIT tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. All nine stage acceptances remain open.

## Flash-only leading whitespace — v0.415

Loaded parseFloat and AVM1 numeric string conversion (including Number and
ToNumber actions) now trim only U+0009, U+000A, U+000D and U+0020, matching the
pinned [Flash whitespace predicate](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/wstr/src/utils.rs)
used by [WStr trim_start](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/wstr/src/common.rs).
This supersedes v0.413's retained broad whitespace policy. VT/FF, NBSP, BOM,
Unicode separators, NUL and lone surrogates stop trimming. Invalid numeric input
still becomes zero for SWF4 numeric conversion and NaN for SWF5–8; parseFloat and
parseInt return NaN in all tested loaded versions. Host API whitespace is unchanged.

`avm1_numeric_whitespace` checks 30 units in SWF4–8 through Number, both prefix
parsers and real ToNumber actions, with traced-string collection between cases.
Its SWF4 numeric assertion failed on v0.414 before implementation.
`jit_numeric_whitespace` checks rejected classes in interpreted, attached native
and automatic native execution, asserting actual native entries for ToNumber and
parseFloat. Other numeric syntax/rounding and complete Flash compatibility remain
open; the comparison is source-pinned, not a new Adobe Player execution.

Validation v0.415 / Проверка: minimal Release 829/829; AGG Release 9/9 and
heap/global ASan+UBSan 9/9 parsing/whitespace/JIT tests. Native mode-restoring
launchers used; stack ASan/LSan disabled. Other-platform acceptance remains open.

## Shared decimal Number conversion — v0.416

Loaded Number, ToNumber and numeric AVM1 operands now use the same allocation-free
counted decimal accumulator as parseFloat. Strict SWF5–8 numeric conversion rejects
trailing grammar; SWF4 retains prefix parsing and invalid-to-zero fallback.
Raw octal/hex selection for SWF6–8 remains separate and unchanged. Optional work
charging is passed by parseFloat; generic numeric coercion retains its previous
uncharged policy. The old host mantissa token and formatted exponent temporary
are removed, so primitive decimal coercion no longer allocates those buffers.
Unloaded host conversion keeps its existing parser.

The reference-pinned exact `.1499999` value now also matches Number/ToNumber;
SWF4 multiple-dot accumulation, exponent wrap, incomplete exponent, negative zero,
counted NUL and strict rejection are checked by `avm1_decimal_numbers`. The exact
rounding assertion failed on v0.415 before this change. `avm1_decimal_allocation`
denies all C++ allocations during direct primitive conversion and checks the
shared helper's strict/relaxed and invalid argument contracts. `jit_decimal_numbers`
checks Number, ToNumber and subtraction across interpreted/attached/automatic
native execution in SWF4–8, with actual native entry assertions. The Ruffle value.rs
reference and MIT attribution are retained in src/avm1/decimal.cpp. Full binary64
rounding, extreme overflow behavior and general coercion work accounting remain open.

Validation v0.416 / Проверка: minimal Release 832/832; AGG Release 12/12 and
heap/global ASan+UBSan 12/12 parsing/decimal/whitespace/JIT tests. Native launchers
restore workspace executable modes; stack ASan/LSan disabled. No other-platform
runtime acceptance or complete stage closure is claimed.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `fvm_call` | function | [include/flashvm/value.h:138](../include/flashvm/value.h#L138) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
| `fvm_standard_install` | function | [include/flashvm/standard.h:20](../include/flashvm/standard.h#L20) |
<!-- source-links:end -->
