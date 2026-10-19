# AS3 syntax frontend 0.1.0

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
