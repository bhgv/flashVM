# LIMBO syntax frontend

Version 0.4.1. [Русская версия](README.ru.md).

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
Polymorphism/exception syntax is supported in 0.4.1; type/context checks are not implemented. Qualifier lookahead is not a general semantic disambiguator;
complex brace-containing qualifier expressions need additional work. Include files
are recognized as directives and are not opened or expanded. Numeric range/base
validation and Unicode identifier categories require later semantic validation.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[letter](limbo.atg#L175) | [trail](limbo.atg#L181) | [lead2](limbo.atg#L186) | [lead3](limbo.atg#L191) | [lead4](limbo.atg#L196) | [ascii](limbo.atg#L201) | [hex](limbo.atg#L206) | [digit](limbo.atg#L211) | [alpha](limbo.atg#L216) | [dq](limbo.atg#L221) | [sq](limbo.atg#L226) | [linechar](limbo.atg#L231) | [escaped](limbo.atg#L236) | [ident](limbo.atg#L242) | [number](limbo.atg#L251) | [radix](limbo.atg#L279) | [text](limbo.atg#L299) | [character](limbo.atg#L309) | [linecomment](limbo.atg#L336) | [Limbo](limbo.atg#L348) | [Top](limbo.atg#L359) | [NamedTop](limbo.atg#L366) | [DeclarationTail](limbo.atg#L380) | [DeclType](limbo.atg#L391) | [AdtMember](limbo.atg#L425) | [Tags](limbo.atg#L440) | [Names](limbo.atg#L447) | [Name](limbo.atg#L455) | [Type](limbo.atg#L462) | [Parameters](limbo.atg#L491) | [Formal](limbo.atg#L510) | [FunctionTail](limbo.atg#L524) | [Block](limbo.atg#L533) | [Statement](limbo.atg#L544) | [LabeledStatement](limbo.atg#L595) | [IfStatement](limbo.atg#L604) | [WhileStatement](limbo.atg#L612) | [ForStatement](limbo.atg#L626) | [CaseStatement](limbo.atg#L644) | [AltStatement](limbo.atg#L653) | [CaseArm](limbo.atg#L663) | [PickStatement](limbo.atg#L673) | [PickArm](limbo.atg#L682) | [ArmBody](limbo.atg#L696) | [Qualifiers](limbo.atg#L705) | [Qualifier](limbo.atg#L713) | [Expression](limbo.atg#L722) | [Assignment](limbo.atg#L731) | [Cons](limbo.atg#L763) | [LogicalOr](limbo.atg#L773) | [LogicalAnd](limbo.atg#L781) | [BitOr](limbo.atg#L789) | [BitXor](limbo.atg#L797) | [BitAnd](limbo.atg#L805) | [Equality](limbo.atg#L813) | [Relational](limbo.atg#L825) | [Shift](limbo.atg#L839) | [Additive](limbo.atg#L851) | [Product](limbo.atg#L863) | [Unary](limbo.atg#L878) | [ArrayCreation](limbo.atg#L918) | [InitializerList](limbo.atg#L947) | [Initializer](limbo.atg#L958) | [Postfix](limbo.atg#L967) | [Subscript](limbo.atg#L986) | [Primary](limbo.atg#L1009)

## Language reference

[Limbo language reference, syntax summary](https://www.inferno-os.org/inferno/papers/limbo.html).

Added labels for while/do/for/case/alt/pick, list of, unary *, checked
array forms and slices with optional bounds. See [coverage](../README.md).

## Syntax 0.4.1

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

New rules / [PolyParameters](limbo.atg#L1024) | [ForPoly](limbo.atg#L1065) | [Raises](limbo.atg#L1089) | [FixedType](limbo.atg#L1119) | [Export](limbo.atg#L1160)

[Parser coverage 0.5.2](../../PARSER_COVERAGE.md) documents XML/cast fixes and remaining boundaries.

C helpers in the authoritative ATG: [NamedCastAhead](limbo.atg#L89).
