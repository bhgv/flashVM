# LIMBO syntax frontend

Version 0.1.0. [Русская версия](README.ru.md).

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

| Driver function | Contract |
| --- | --- |
| [main](main.c#L97) | One input path; opens/closes the descriptor; returns 0 for syntax acceptance, 1 for syntax/encoding rejection, 2 for usage/I/O failure |
| [validate_source](main.c#L33) | Validates encoding without modifying source bytes; returns 0/1/2 and rewinds the descriptor on success |
| [report_error](main.c#L85) | Synchronous generated-parser callback; prints path, line, byte column, diagnostic text/code and byte offset; owns no memory |

The driver uses generated `Parse`, `Successful` and `Custom_Error`; these are
upstream generated interfaces, not a stable public compiler API. Error recovery
may stop at the first useful diagnostic; process status is authoritative. A syntax
acceptance does not prove valid types, assignable left operands, valid modifiers,
control-flow placement or any bytecode/SWF compatibility.

## Syntax coverage and boundaries

Recognizes implementation/interface units, includes, module and ADT declarations,
pick variants, function definitions, grouped parameters, primitive/composite types,
constants, imports, tuples, arrays/slices, channel creation/send/receive, spawn,
case/alt/pick, loops, calls and expression precedence. AheadArrow stops an arm body
before the next outer qualifier without consuming input; strings, comments and
parenthesis/subscript nesting are skipped. Empty branch bodies are intentional.

This is an initial profile of the classic language, not a complete Inferno compiler.
Loop labels, newer polymorphism/exception syntax and all type/context checks are
not implemented. Qualifier lookahead is not a general semantic disambiguator;
complex brace-containing qualifier expressions need additional work. Include files
are recognized as directives and are not opened or expanded. Numeric range/base
validation and Unicode identifier categories require later semantic validation.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[Limbo](limbo.atg#L94) | [Top](limbo.atg#L99) | [NamedTop](limbo.atg#L106) | [DeclarationTail](limbo.atg#L112) | [DeclType](limbo.atg#L118) | [AdtMember](limbo.atg#L129) | [Tags](limbo.atg#L137) | [Names](limbo.atg#L141) | [Name](limbo.atg#L146) | [Type](limbo.atg#L153) | [Parameters](limbo.atg#L167) | [Formal](limbo.atg#L172) | [FunctionTail](limbo.atg#L178) | [Block](limbo.atg#L183) | [Statement](limbo.atg#L190) | [IfStatement](limbo.atg#L208) | [WhileStatement](limbo.atg#L214) | [ForStatement](limbo.atg#L220) | [CaseStatement](limbo.atg#L226) | [AltStatement](limbo.atg#L231) | [CaseArm](limbo.atg#L237) | [PickStatement](limbo.atg#L244) | [PickArm](limbo.atg#L249) | [ArmBody](limbo.atg#L256) | [Qualifiers](limbo.atg#L262) | [Expression](limbo.atg#L267) | [Assignment](limbo.atg#L273) | [Cons](limbo.atg#L279) | [LogicalOr](limbo.atg#L284) | [LogicalAnd](limbo.atg#L289) | [BitOr](limbo.atg#L294) | [BitXor](limbo.atg#L299) | [BitAnd](limbo.atg#L304) | [Equality](limbo.atg#L309) | [Relational](limbo.atg#L314) | [Shift](limbo.atg#L319) | [Additive](limbo.atg#L324) | [Product](limbo.atg#L329) | [Unary](limbo.atg#L336) | [Initializer](limbo.atg#L346) | [Postfix](limbo.atg#L352) | [Primary](limbo.atg#L358)

## Language reference

[Limbo language reference, syntax summary](https://www.inferno-os.org/inferno/papers/limbo.html).
