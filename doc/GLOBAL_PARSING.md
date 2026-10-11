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
