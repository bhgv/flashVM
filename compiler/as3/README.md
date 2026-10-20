# AS3 syntax frontend 0.2.1

`frontend/as3.atg` is the authoritative Coco/R parser source; the driver is C99.
This is an AS3 syntax subset, without AST, type checking or ABC generation.
It accepts package/import, class/interface, extends/implements, modifiers,
var/const, signatures, blocks, loops, exceptions and is/as expressions.
E4X/XML, metadata, namespaces, Vector.<T> and regular expressions are unsupported.
Modifier combinations, return and const validity need a future semantic pass.
The separate AS3 → FIR → AVM1 tutorial lives in `../../fir/v1`; it is not AVM2.

```sh
cmake -S compiler/as3 -B build/as3 -DCOCOR_ROOT="$COCO_DIR" -DCOCOR_EXECUTABLE="$COCO_DIR/sources/cocor"
cmake --build build/as3
ctest --test-dir build/as3 --output-on-failure
build/as3/frontend/as3_parse example.as
```

External dependency: [bhgv/coco-r-C](https://github.com/bhgv/coco-r-C).

Examples: `examples/Counter.as` and `examples/ICounter.as` exercise class and
interface productions. Run each with `as3_parse`. Functions and parameters use
FunctionTail/Parameter; operators use the precedence chain Assignment through
Primary; type declarations use TypeDeclaration/Member/Signature. Diagnostics
include file, byte-based line/column and generated Coco/R error number. Status:
0 syntax accepted, 1 rejected, 2 command/I/O failure. UTF-8 and NUL are validated
before scanning. A detached renamed `as3` directory builds without siblings.

CLI: `--help`, `--version` (no source file required; status 0).

## C API

`frontend/frontend.h`: `as3_check(bytes, size, diagnostic)` and
`as3_read(FILE*, diagnostic)` return 1 accepted / 0 rejected. The caller owns
the input buffer/stream; the library prints nothing, closes no input and never
exits. Diagnostic fields: code, line/column, byte offset, message. NULL diagnostics
are allowed. Maximum input: 2 MiB. CMake target: `as3_frontend`. Generated global
symbols have a language prefix; multiple libraries can link into one IDE. State
resets before every call, but each parser requires external serialization (not
reentrant). `tests/embed.c` checks in-process recovery after invalid input.

### in operator

Relational `in` and for-in/classic-for disambiguation are supported. The initial
for clause uses NoIn; groups, arguments, subscripts and literals locally restore
ordinary expressions. Function bodies own their expression context. Productions:
Relational, ForStatement, GroupedExpression, Arguments, Subscript. This is syntax
recognition, not property enumeration or FIR lowering.

[Grammar](frontend/as3.atg): Relational/ForStatement. [Example](examples/in.as).
