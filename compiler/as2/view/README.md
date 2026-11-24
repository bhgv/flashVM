# AS2 display profile → FIR → SWO → SWF

[Русский](README.ru.md) · [Shared AS2 frontend](../frontend/README.md) ·
[Library API](view.h) · [Coco/R profile grammar](view.atg) ·
[Executed IDE source](../../../ide/as/editor-view.as) ·
[FIR operations](../../../fir/v1/doc/AS_PROPERTIES.md)

The AS2 project now has a bounded executable profile through the common pipeline:

**AS2 frontend → FIR 1.0 → verification/peepholes → target compiler → SWO → SWL → SWF.**

`as2_check` validates the full input before the generated Coco/R profile parser
lowers supported constructs to textual FIR. This frontend contains no AVM1 opcode
emitter. Existing FIR operations handle constants, coercion and arithmetic.
Dynamic AS operations handle globals, member stores and method calls. The FIR AVM1 backend emits
target instructions, and the existing SWL linker creates the SWF container.

The IDE embeds the linked SWF and executes its actions in FlashVM for every
complete view. No compiler, parser, Node or Python is needed by the running IDE.
Dynamic Unicode strings and owner-local UI objects are bound in its FlashVM
context; Dis passes checked values, never native heap addresses.

## Implemented source profile

Nonempty sequences of explicit-semicolon assignments are supported:
`object.property = expression;` and `global = expression;`. Expressions support
global reads, nonnegative decimal int32 constants, booleans, double-quoted UTF-8
strings, and named method calls with zero to eight arguments. The receiver is
evaluated first, then arguments from left to right. The operators `+ - * /` use ordinary precedence.
Unary `!` inverts AVM1 truthiness, and parentheses group expressions.
`+` lowers to dynamic `as.add`; the other arithmetic operators explicitly
convert operands with `as.to_number` before f64 arithmetic. Values retain AS
semantics, including strings passed to a property. The LHS object is read before
the RHS is evaluated. Identifiers and separators are ASCII. UTF-8 `//` comments
may end at EOF. The profile uses CR/LF line boundaries.

Braced `if (expression) { ... }` supports an optional braced `else`. The condition
uses AVM1 truthiness. The entire unit can be a parameterless
`function name() { ... }`. The SWF initializer installs that method on
the root MovieClip. FlashVM calls `onEnterFrame` on timeline ticks, and a script
can call another named method on `_root`. SSA temporaries are
local to each invocation. Plain assignments follow normal AVM1 name resolution.

Parameters in declarations, standalone call statements, member reads, loops
and classes remain outside the profile. The shared syntax frontend
can recognize more than this profile; unsupported lowering fails explicitly.
Bounds: source 16 KiB, names 253 bytes, spelled string contents 255 bytes,
staged FIR 64 KiB. Quoted strings support `\"`, `\\`, `\n`, `\r` and `\t`. Assignment comments
retain source line/column/byte positions; they are not full debugger source maps.
The profile currently targets AVM1/SWF. Dis rejects these AS object operations
until a typed foreign-object adapter/backend exists; it does not reinterpret them.

## Build and run the complete pipeline

From the repository root, after building the bundled Coco/R generator:

```sh
# Build the AS2 frontend and its CLI/library profile together.
cmake -S compiler/as2 -B build/as2 \
  -DCOCOR_ROOT="$PWD/tools/coco-r-C/upstream" \
  -DCOCOR_EXECUTABLE="$PWD/build/cocor/cocor"
cmake --build build/as2 -j4
ctest --test-dir build/as2 --output-on-failure
# FIR and SWL remain independently buildable tools.
cmake -S fir/v1 -B build/fir \
  -DCOCOR_ROOT="$PWD/tools/coco-r-C/upstream" \
  -DCOCOR_EXECUTABLE="$PWD/build/cocor/cocor"
cmake --build build/fir -j4
cmake -S linker -B build/linker
cmake --build build/linker -j4
# Every artifact is explicit and inspectable; the frontend never creates SWF.
build/as2/view/as2_viewc ide/as/editor-view.as build/editor-view.fir
build/fir/fir1 check build/editor-view.fir
build/fir/fir1_opt build/editor-view.fir build/editor-view.opt.fir
build/fir/fir1 object build/editor-view.opt.fir build/editor-view.swo
build/linker/swl link build/editor-view.swf build/editor-view.swo
```

The root `build-workspace.sh --projects disvm-editor` builds/reuses AS2, FIR and
SWL first. Direct editor CMake takes explicit `DVM_AS_VIEW_COMPILER`,
`DVM_FIR_COMPILER`, `DVM_FIR_OPTIMIZER`, `DVM_SWF_LINKER` paths. Its native build
retains `.fir`, `.opt.fir`, `.swo`, `.swf` under `native/`; only the linked container
is embedded by the byte-copy resource tool. That tool parses no source language
and generates no target instructions.

`compiler/as2/view` also builds separately with explicit Coco/R paths, using
`../frontend`. Detach the whole AS2 language project, not this directory alone.

## Library/CLI and failure contracts

Target `as2_view`, API `as2_view_compile` and CLI `as2_viewc` share the same code.
The caller owns input/output/diagnostic and serializes calls with the shared AS2
frontend. The library performs no file I/O or subprocess calls and retains no
input pointer. Failure sets `written=0`, leaves output unchanged, and recovers
scanner allocation failure at the API boundary. Internal buffers are reused.
It is not reentrant or concurrently callable. The CLI writes FIR bytes, with no
terminating NUL in the product. Failed parsing leaves the old file unchanged;
callers must honor its nonzero exit status. The IDE build stops on any stage error.

## Evidence and remaining scope

The embedding test checks nested calls, UTF-8 strings, failure recovery and
atomic output on capacity failure. FIR tests check operation types, effect ordering, canonical roundtrip and
Dis rejection. `disvm_as_view_execution` loads the **linked SWF** into FlashVM,
changes geometry and checks Unicode values across GC. Graphical browser replay
checks actual TextField pixels, row highlighting and native mouse actions. These
are separate from physical desktop/touch acceptance. The [paired terminal](../../../ide/examples/paired-terminal/README.md)
uses this compiler to receive and reply on `styxTerminal` and verifies the final pixels.

General AS AST/FIR lowering, function bodies, full source maps and migration of
remaining C-rendered editor widgets stay open. Document/browser policy is Limbo;
AS handles the implemented TextField content/geometry; native font/raster/window
and OS ports provide primitives.

Initializer and `onEnterFrame` units use `@ide_view`. Other root methods use
`@ide_view_<name>` so their SWO objects can be linked together. Root method names
are limited to 244 ASCII bytes to fit the generated module name.
