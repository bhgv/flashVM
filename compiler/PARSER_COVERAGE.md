# Parser coverage and boundaries, series 0.5.2

This series substantially extends all five syntax interfaces, but does not claim
complete compilers or full language conformance. Source-language parsers do not
build ASTs, resolve names or check types. Syntax recognition, executable FIR and
remaining frontend work are explicitly separated below.

| Component | Added and checked | Remaining before a complete frontend |
| --- | --- | --- |
| AS1/AS2 | ASI/restricted line breaks; and/or/not; on/onClipEvent; include/initclip wrappers | Escaped identifiers, Unicode categories/exact Flash profile, AST/context/assignment targets; include expansion separately |
| AS3 | ASI, metadata, nested Vector, regexp lexical goal, XML/XMLList/E4X and real AS interpolations | Full regexp validation, XML namespace/entity resolution, Unicode/escapes, remaining directives/lexical cases and AST/context |
| Limbo | Polymorphism/constraints, exceptions, fixed, **, slices and type arguments | Complete symbol-based type/value disambiguation, remaining qualifier ambiguities and AST/context |
| FIR | Multiple signatures, block/edge parameters, invoke, declarations/debug records and composite types | Extended IR construction/verification, opcode/effect catalog and complete lowering of new constructs |

AS1 is 0.3.0, AS2 0.3.1, AS3/Limbo 0.4.1, FIR tools 0.5.0, independent FIR
syntax API 0.1.0. The text header remains FIR 1.0; ABI/component versions are
independent of the player. ATG files are authoritative sources; external
bhgv/coco-r-C generates C99 into build directories. C actions end their physical
line and are aligned by the formatter; production periods have their own line
and a following blank separator.

## C interfaces

AS/Limbo expose *_check(bytes,size,diagnostic) and *_read(FILE*,diagnostic).
The new FIR API is firsyn_check/firsyn_read in firsyn.h. All return 1/0, retain
host input ownership, print nothing and never exit. Diagnostics are host-owned
and may be NULL. Global names have separate prefixes. Serialize access to each
parser; sequential recovery after errors is supported. This is not a promise of
reentrancy or thread safety of one API.

```sh
cmake -S compiler -B build-five \
  -DCOCOR_ROOT=/host/coco-r-C \
  -DCOCOR_EXECUTABLE=/host/coco-r-C/sources/cocor \
  -DFIR_SYNTAX_ROOT=/absolute/path/to/fir/v1/frontend
cmake --build build-five
ctest --test-dir build-five --output-on-failure
```

FIR_SYNTAX_ROOT is an explicit optional SDK/source root. Without it, compiler
still builds four independent languages and never searches a sibling FIR tree.
The additional compilers_five_embedding test links all five libraries into one
C99 process, alternates errors/successes and uses no CLI/subprocess adapter.

## Validation and evidence limits

0.5.2 baseline: GitHub master 1c5c8c49833e139191d50d6f4ffacca78f57f43f. Earlier 0.5.1 used 54f324c5b37c7cbbe65c58f3fb774cf01099b776.
On Linux x86-64, 19 compiler CTest groups and 6 FIR groups passed, including real
C APIs and the syntax/backend boundary. The five-API host was checked separately.
Renamed detached copies of compiler and fir/v1 also built and passed their tests;
repeat runs in copies are not additional scenarios. External Coco/R code was
neither changed nor included in this repository.

All five CLIs and fir1 cross-built for ARM64 Linux/musl, RISC-V64 Linux/musl and
x86-64 Windows/GNU; ELF/PE headers and machine IDs were checked. These builds used
host-adapted Zig 0.13, not acceptance of an unmodified SDK. Foreign binaries were
not executed; there were no new macOS/Android runs or sanitizer runs.
Eight upstream Inferno files (sys/draw/math/string/bufio.m, echo/cat/ls.b) were
accepted syntactically, not type checked, include-expanded or executed.

Previous [VALIDATION.md](VALIDATION.md) results belong to their recorded version;
its counts, sanitizer and VM evidence are not attributed to this series.

## Sources / Исходники

| Parser | Grammar | C interface | Examples |
| --- | --- | --- | --- |
| AS1 | [as1.atg](as1/frontend/as1.atg) | [frontend.h](as1/frontend/frontend.h) | [README](as1/examples/README.md) |
| AS2 | [as2.atg](as2/frontend/as2.atg) | [frontend.h](as2/frontend/frontend.h) | [README](as2/examples/README.md) |
| AS3 | [as3.atg](as3/frontend/as3.atg) | [frontend.h](as3/frontend/frontend.h) | [README](as3/examples/README.md) |
| Limbo | [limbo.atg](limbo/frontend/limbo.atg) | [frontend.h](limbo/frontend/frontend.h) | [README](limbo/examples/README.md) |
| FIR | [firsyn.atg](../fir/v1/frontend/firsyn.atg) | [firsyn.h](../fir/v1/frontend/firsyn.h) | [README](../fir/v1/examples/structural/README.md) |

[Five-API host](tests/embed_five.c) · [Build guide](../BUILDING.md) ·
[FIR executable format](../fir/v1/doc/FORMAT.md) ·
[Inferno reference grammar](https://github.com/inferno-os/inferno-os/blob/master/limbo/limbo.y)

## Changes in 0.5.2

AS3 0.4.1 validates Unicode XML NameStartChar/NameChar independently of locale,
entity reference spelling, numeric reference ranges, comments, PI, forbidden
characters and `]]>` in text. XMLList requires an empty closing name.
Contextual `include "file.as"` is recognized without reading a host file;
ordinary identifiers named `include` remain usable. This is an XML 1.0
fifth-edition lexical profile, not a claim of exact Adobe E4X compatibility.

Limbo 0.4.1 accepts `Point value`, `m->Point value`, cast chains and
conversion expressions inside export/index brackets. `list of {...}` remains
a value expression there. General type/value disambiguation still needs work. Named cast prefixes may
include type arguments at each qualification level. AST/type/lvalue checks and complete regexp
Pattern validation were not added. FIR and executable backends are unchanged.
Complete conformance of all five languages is still unproven; this release
closes the specific gaps listed here.

New fixtures: [XML](as3/examples/XmlReferences.as),
[Limbo casts](limbo/examples/NamedCasts.b). Build instructions appear above.

0.5.2 validation: 19 native compiler CTest groups and 6 FIR groups; the same
tests in renamed detached directories; five APIs in one C99 host. Compiler/FIR
cross-builds were repeated for ARM64 Linux/musl, RISC-V64 Linux/musl and
Windows x86-64/GNU, checking ELF/PE machine IDs without foreign execution.
Updated tests cover XML references, Unicode names, controls, every byte-prefix
truncation of XML fixtures, C API error/recovery, include, named and instantiated
casts, exports and preservation of ordinary arithmetic expressions.
XML rules: [W3C XML 1.0](https://www.w3.org/TR/xml/);
cast syntax: [Inferno limbo.y](https://github.com/inferno-os/inferno-os/blob/master/limbo/limbo.y).
