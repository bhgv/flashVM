# LIMBO syntax frontend

Version 0.5.2. [Русская версия](README.ru.md).

The readable [limbo.atg](limbo.atg) is the authoritative scanner/parser input.
It targets the C backend of [bhgv/coco-r-C](https://github.com/bhgv/coco-r-C).
The embedded predicates are C99; the executable and generated code are built as
C99 with extensions disabled. This is a syntax recognizer, not yet a complete
compiler: there is no AST, symbol table, type checker, code generator or SWF writer.

## Build and detach

Provide an already-built external C generator and its matching `frames/` directory.
No Coco/R source, frame or documentation is copied into this repository. Run from
this frontend directory:

```sh
cmake -S . -B build \
  -DCOCOR_ROOT=/absolute/path/to/coco-r-C \
  -DCOCOR_EXECUTABLE=/absolute/path/to/coco-r-C/cocor
cmake --build build
ctest --test-dir build --output-on-failure
./build/limbo_parse /path/to/source
```

The executable needs no generator at runtime. Python 3 is required only for tests;
use `-DBUILD_TESTING=OFF` for a build without it. CMake 3.16+ and a C99 compiler are
required. Windows multi-configuration builds place the executable in the selected
configuration directory (for example `build/Debug`). Windows was not run here.
A compiler-wide optional build is in [../../CMakeLists.txt](../../CMakeLists.txt).
The same configuration options work from the language project directory.

This directory can be copied or renamed and configured without FlashVM, linker,
other frontends or workspace helpers. Keep the common root MIT LICENSE in a
separated distribution; generated files also carry their upstream frame notices.
Generated C and headers live only in `build/generated/` and are not source commits.
Each frontend also exposes a namespaced C library. Sequential calls reset
state; serialize calls to the same parser. See the language README for C API.

## Driver and diagnostics

[main.c](main.c) opens one source file and delegates to [api.c](api.c),
which validates UTF-8 before parsing. Malformed UTF-8, surrogates, overlong encodings, values
above U+10FFFF, incomplete sequences and embedded NUL are rejected. An initial
UTF-8 BOM is supported by the selected generator. Inputs must be seekable files.
The scanner counts **bytes**, not Unicode columns; identifier category restrictions
and escape-value semantics remain future validation work. Strings preserve bytes.

Public interface: [frontend.h](frontend.h), [api.c](api.c).

`limbo_check(bytes, size, diagnostic)` / `limbo_read(FILE*, diagnostic)`
return 1 accepted / 0 rejected; diagnostic may be NULL. Input storage and
streams belong to the caller. Serialize calls to one parser; calls after
failure are supported. Input is limited to 2 MiB UTF-8. Codes -3/-4 indicate
nesting/work exhaustion; they are implementation limits, not language errors.
Coordinates may be zero on resource failure. Syntax recognition does not
replace type, symbol or return/break context analysis.

## Syntax coverage and boundaries

Recognizes implementation/interface units, includes, module and ADT declarations,
pick variants, function definitions, grouped parameters, primitive/composite types,
constants, imports, tuples, arrays/slices, channel creation/send/receive, spawn,
case/alt/pick, loops, calls and expression precedence. AheadArrow stops an arm body
before the next outer qualifier without consuming input; strings, comments and
parenthesis/subscript nesting are skipped. Empty branch bodies are intentional.

This is an initial profile of the classic language, not a complete Inferno compiler.
Polymorphism/exception syntax is supported in 0.4.2; type/context checks are not implemented. Qualifier lookahead is not a general semantic disambiguator;
initializer braces after of are balanced by the outer-arrow resolver. Include files
are recognized as directives and are not opened or expanded. Numeric range/base
validation and Unicode identifier categories require later semantic validation.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[letter](limbo.atg#L198) | [trail](limbo.atg#L204) | [lead2](limbo.atg#L209) | [lead3](limbo.atg#L214) | [lead4](limbo.atg#L219) | [ascii](limbo.atg#L224) | [hex](limbo.atg#L229) | [digit](limbo.atg#L234) | [alpha](limbo.atg#L239) | [dq](limbo.atg#L244) | [sq](limbo.atg#L249) | [linechar](limbo.atg#L254) | [escaped](limbo.atg#L259) | [ident](limbo.atg#L265) | [number](limbo.atg#L274) | [radix](limbo.atg#L302) | [text](limbo.atg#L322) | [character](limbo.atg#L332) | [linecomment](limbo.atg#L359) | [Limbo](limbo.atg#L371) | [Top](limbo.atg#L382) | [NamedTop](limbo.atg#L389) | [DeclarationTail](limbo.atg#L403) | [DeclType](limbo.atg#L414) | [AdtMember](limbo.atg#L448) | [Tags](limbo.atg#L463) | [Names](limbo.atg#L470) | [Name](limbo.atg#L478) | [Type](limbo.atg#L485) | [Parameters](limbo.atg#L514) | [Formal](limbo.atg#L533) | [FunctionTail](limbo.atg#L547) | [Block](limbo.atg#L556) | [Statement](limbo.atg#L567) | [LabeledStatement](limbo.atg#L618) | [IfStatement](limbo.atg#L627) | [WhileStatement](limbo.atg#L635) | [ForStatement](limbo.atg#L649) | [CaseStatement](limbo.atg#L667) | [AltStatement](limbo.atg#L676) | [CaseArm](limbo.atg#L686) | [PickStatement](limbo.atg#L696) | [PickArm](limbo.atg#L705) | [ArmBody](limbo.atg#L719) | [Qualifiers](limbo.atg#L728) | [Qualifier](limbo.atg#L736) | [Expression](limbo.atg#L745) | [Assignment](limbo.atg#L754) | [Cons](limbo.atg#L786) | [LogicalOr](limbo.atg#L796) | [LogicalAnd](limbo.atg#L804) | [BitOr](limbo.atg#L812) | [BitXor](limbo.atg#L820) | [BitAnd](limbo.atg#L828) | [Equality](limbo.atg#L836) | [Relational](limbo.atg#L848) | [Shift](limbo.atg#L862) | [Additive](limbo.atg#L874) | [Product](limbo.atg#L886) | [Unary](limbo.atg#L901) | [ArrayCreation](limbo.atg#L941) | [InitializerList](limbo.atg#L970) | [Initializer](limbo.atg#L981) | [Postfix](limbo.atg#L990) | [Subscript](limbo.atg#L1009) | [Primary](limbo.atg#L1032)

## Language reference

[Limbo language reference, syntax summary](https://www.inferno-os.org/inferno/papers/limbo.html).

Added labels for while/do/for/case/alt/pick, list of, unary *, checked
array forms and slices with optional bounds. See [coverage](../README.md).

## Syntax 0.4.2

Adds multiple implement modules, polymorphic function/ADT/type parameters and
for constraints, exception declarations/raises/raise/block handlers, fixed types
and conversions, **/**=, array of byte conversion, trailing list comma and omitted
slice bounds. Variadic * must be last (E1302); empty case/alt/pick and empty for
constraints are rejected. Comparison with current Inferno corrected the former
lower-bound restriction: a[:n] and a[:] are legal.
Includes are not expanded. Named and instantiated type casts are recognized. Symbol/type resolution and
complete type/value disambiguation in polymorphic arguments still need work. This is not a complete
Inferno compiler or Dis generator.

[Polymorphic example](../examples/Polymorphic.b), [upstream grammar](https://github.com/inferno-os/inferno-os/blob/master/limbo/limbo.y).

New rules / [PolyParameters](limbo.atg#L1047) | [ForPoly](limbo.atg#L1088) | [Raises](limbo.atg#L1112) | [FixedType](limbo.atg#L1142) | [Export](limbo.atg#L1183)

[Parser coverage 0.6.0](../../PARSER_COVERAGE.md) documents XML/cast fixes and remaining boundaries.

C helpers in the authoritative ATG: [NamedCastAhead](limbo.atg#L112).

[Lexical profile 0.6.0](../../LEXICAL_PROFILE.md).

0.6.0: [AheadArrow](limbo.atg#L18).
