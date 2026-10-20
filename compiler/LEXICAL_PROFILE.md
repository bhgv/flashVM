# Parser lexical profile, compiler series 0.6.0

[Русский](LEXICAL_PROFILE.ru.md) · [Coverage](PARSER_COVERAGE.md) ·
[Native and cross builds](../BUILDING.md) · [Examples](examples.md).

## ActionScript identifiers and source locations

AS1/AS2 0.4.0 and AS3 0.5.0 accept UTF-8 identifiers and exact `\uXXXX`
escapes. The initial scalar is `$`, `_` or General_Category Lu/Ll/Lt/Lm/Lo/Nl.
Subsequent scalars additionally admit Mn/Mc/Nd/Pc and U+200C/U+200D.
Category data is pinned to **Unicode 3.2.0**; later Unicode additions are not
silently enabled by the host libc. This is an explicit lexical profile, not a
claim of identical behavior across every Adobe Flash release. Escaped surrogate
code units are invalid identifier characters; direct supplementary UTF-8 letters
present in the pinned database are accepted. Normalization and identifier binding
are separate future semantic operations.

The authoritative ATGs describe byte spelling and scalar checks. Each detached
frontend contains its own identical `unicode_ident.inc`. Regenerate and check:

```sh
python3 tools/generate_parser_unicode.py
python3 tools/generate_parser_unicode.py --check
python3 tools/atg_format.py --check compiler/*/frontend/*.atg
```

The generator uses Python's immutable `unicodedata.ucd_3_2_0`, checks the reserved
keyword list against grammar literals and is a maintainer tool only. C99 builds
consume checked-in data and need no Python/ICU/wchar_t Unicode implementation.
An escaped reserved word cannot become a variable, parameter or expression name.
AS3 property IdentifierName syntax remains separate: `obj.\u0069f` and
`{\u0069f: 1}` are accepted, while `var \u0069f=1` is rejected.

Unicode spaces, BOM-as-whitespace, VT/FF and LS/PS no longer become parts of an
identifier merely because the byte scanner admits high bytes. Source bytes are
never rewritten. Diagnostic offsets remain original byte offsets. Columns count
UTF-8 bytes with eight-column tabs; CRLF counts once and CR/LF/LS/PS each advance
a line. The location cursor is monotonic and also visits text skipped by raw
regexp/XML goals. Single-line comments stop at all four line terminators.
Contextual label/metadata/type lookahead observes the same whitespace boundaries.

## String literals

Quoted strings validate `\xHH` and `\uHHHH` spelling. String Unicode escapes
preserve UTF-16 code units, so a surrogate escape is permitted in a string even
though it is forbidden in an identifier. Escaped CR, LF, CRLF, LS and PS form
line continuations. Unescaped line terminators are rejected. This parser does
not evaluate strings, decode values into an AST or validate numeric conversions.

## AS3 regexp syntax

Operand-goal `/.../gimsx` is separate from division and `/=`. The ATG now parses
alternation, empty alternatives, captures, noncapturing groups, lookahead,
lookbehind/atomic groups, AS3 `(?P<name>...)`, numeric/named backreferences,
character classes and ordered ranges, control/hex/Unicode escapes, assertions,
quantifier bounds and lazy/possessive suffixes. Extended `x` mode handles ASCII
pattern whitespace and `#` comments outside character classes. Arbitrarily long
decimal bounds are compared as digit spans, without host integer overflow.

Bad delimiters, malformed groups/escapes, duplicate names, missing referenced
groups, descending ranges, assertions with quantifiers and repeated quantifiers
are rejected with E1103. Group recursion uses the parser nesting guard. At most
512 named definitions and 512 named references are retained in one pattern;
numeric references use the documented range 1..99. This is a bounded structural
parser, not a regexp compiler or matcher. It does not check fixed-width
lookbehind or every PCRE engine extension/compatibility behavior. Inline PCRE
option groups and malformed-brace-as-literal compatibility are not part of this
profile. Acceptance must not be reported as proof that Adobe's regexp engine
will compile or execute the same pattern.

[Group reference](https://airsdk.dev/docs/development/core-actionscript-classes/using-regular-expressions/regular-expression-syntax/groups) ·
[Quantifiers](https://airsdk.dev/docs/development/core-actionscript-classes/using-regular-expressions/regular-expression-syntax/quantifiers) ·
[ECMA lexical rules](https://262.ecma-international.org/5.1/#sec-7.6).

## AS3 directives, vectors and Limbo qualifiers

AS3 recognizes `new <int>[1,2]`, nested element types, empty initializers and a
single trailing comma, followed by normal index/property/call syntax. Typed
initializers reject array holes. `CONFIG::debug { ... }` blocks are accepted at
item scope, and class guards contain members. Guard values are not evaluated;
namespace definitions and binding are separate semantic work. The existing
contextual `include "file.as"` still does not read arbitrary host files.

Limbo's outer-arrow resolver balances initializer braces after `of`, as well as
parentheses and brackets. It ignores arrows inside strings/comments and nested
array initializers, without consuming parser tokens. This permits a later
case/alt qualifier to contain a list literal or parenthesized array expression.
Qualifier constness, channel direction, types and type/value binding remain
semantic checks. Limbo retains its own lexical profile and does not acquire AS
Unicode escapes or AS semicolon insertion.

## Generator boundary and cross builds

All five standalone syntax projects delete their previous generated outputs
before invoking the external host Coco/R, then check exit status, diagnostics and
all expected files. A regression test starts with stale files and supplies an
invalid grammar. A failed build is never a successful test of newly generated
code; tests must run only after a successful build command.

The inspected Coco/R master b91b652 already returns failure on grammar syntax
errors. No exit-code fix is required. A separate confirmed fix renames C error
parameters `errno` to `error_number` in parser frames and self-hosted sources.
Windows stdlib.h can define `errno` as a pointer-valued macro; the old parameter
name then changes the generated function's type. Use the supplied Coco/R errno
patch or an upstream revision containing it **before Windows cross-generation**.
Rebuild that host generator with `make -C sources -f unix.mk`, and pass its
matching frame root and executable explicitly through CMake. No external Coco/R
source, runtime or documentation is copied into FlashVM.

Compiler/FIR and detached copies are tested independently. Cross binaries target
ARM64 Linux/musl, RISC-V64 Linux/musl and x86-64 Windows/GNU; this series checks
ELF/PE identities rather than claiming foreign execution. The explicit SDK
commands in [BUILDING.md](../BUILDING.md) remain applicable.
