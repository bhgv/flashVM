# AS2 syntax frontend

Version 0.3.0. [Русская версия](README.ru.md).

The readable [as2.atg](as2.atg) is the authoritative scanner/parser input.
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
./build/as2_parse /path/to/source
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

`as2_check(bytes, size, diagnostic)` / `as2_read(FILE*, diagnostic)`
return 1 accepted / 0 rejected; diagnostic may be NULL. Input storage and
streams belong to the caller. Serialize calls to one parser; calls after
failure are supported. Input is limited to 2 MiB UTF-8. Codes -3/-4 indicate
nesting/work exhaustion; they are implementation limits, not language errors.
Coordinates may be zero on resource failure. Syntax recognition does not
replace type, symbol or return/break context analysis.

## Syntax coverage and boundaries

Supported: variables, functions/closures, object/array literals (including array
holes), calls/member/index chains, labels, conditional and loop statements,
for-in, switch, try/catch/finally, with, returns and throws. Expressions have
separate precedence levels; assignment and conditional are right-recursive.
LabelAhead distinguishes labels through whitespace/comments without consuming
scanner state. Explicit predicates resolve block/function statement ambiguity
and bind else to the nearest if. Syntax tests include both valid and invalid input.

Automatic `;` insertion is supported at a line break, before `}` and at EOF.
Return/break/continue and postfix ++/-- respect line breaks, including comments;
after throw a line break is rejected (E1102). For-header semicolons remain mandatory.
Classic on/onClipEvent wrappers, #include and paired #initclip/#endinitclip are
recognized without opening includes or attaching events to a scene. Ordinary
on/onClipEvent calls without a following block retain expression syntax.
Legacy and/or/not operators are supported; event names need profile validation.
Regexp literals, escaped identifiers, complete Unicode-category tables and
assignment-target/context checks remain unimplemented. This syntax profile is
not a complete compiler implementation.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[letter](as2.atg#L158) | [digit](as2.atg#L164) | [hex](as2.atg#L169) | [dq](as2.atg#L174) | [sq](as2.atg#L179) | [linechar](as2.atg#L184) | [escaped](as2.atg#L189) | [nonstar](as2.atg#L195) | [nonstarslash](as2.atg#L199) | [ident](as2.atg#L205) | [number](as2.atg#L214) | [hexnum](as2.atg#L248) | [text](as2.atg#L261) | [blockcomment](as2.atg#L278) | [linecomment](as2.atg#L296) | [As2](as2.atg#L309) | [Item](as2.atg#L317) | [Block](as2.atg#L330) | [Statement](as2.atg#L341) | [IfStatement](as2.atg#L382) | [WhileStatement](as2.atg#L390) | [ForStatement](as2.atg#L397) | [SwitchStatement](as2.atg#L420) | [TryStatement](as2.atg#L429) | [SwitchPart](as2.atg#L445) | [Variables](as2.atg#L456) | [Variable](as2.atg#L464) | [FunctionTail](as2.atg#L475) | [Parameter](as2.atg#L493) | [Expression](as2.atg#L501) | [Assignment](as2.atg#L510) | [AssignOp](as2.atg#L527) | [Conditional](as2.atg#L544) | [LogicalOr](as2.atg#L556) | [LogicalAnd](as2.atg#L568) | [BitOr](as2.atg#L580) | [BitXor](as2.atg#L588) | [BitAnd](as2.atg#L596) | [Equality](as2.atg#L604) | [Relational](as2.atg#L620) | [Shift](as2.atg#L641) | [Additive](as2.atg#L654) | [Product](as2.atg#L668) | [Unary](as2.atg#L682) | [Postfix](as2.atg#L705) | [Arguments](as2.atg#L725) | [Primary](as2.atg#L738) | [Subscript](as2.atg#L764) | [ArrayLiteral](as2.atg#L771) | [ObjectLiteral](as2.atg#L787) | [GroupedExpression](as2.atg#L800) | [Property](as2.atg#L807) | [TypeName](as2.atg#L817) | [Import](as2.atg#L825) | [Class](as2.atg#L838) | [Interface](as2.atg#L860) | [Modifier](as2.atg#L876) | [Member](as2.atg#L885) | [Signature](as2.atg#L901)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.

New rules / [ClipInitializer](as2.atg#L917) | [EventHandler](as2.atg#L930) | [Terminator](as2.atg#L948)
