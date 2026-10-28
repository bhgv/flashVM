# AS2 display profile → FIR → SWO → SWF

[Русский](README.ru.md) · [Shared AS2 frontend](../frontend/README.md) ·
[Library API](view.h) · [Coco/R profile grammar](view.atg) ·
[Executed IDE source](../../../ide/as/editor-view.as) ·
[FIR operations](../../../fir/v1/doc/AS_PROPERTIES.md)

The AS2 project now has a bounded executable profile through the common pipeline:

**AS2 frontend → FIR 1.0 → verification/peepholes → target compiler → SWO → SWL → SWF.**

`as2_check` validates the full input before the generated Coco/R profile parser
lowers supported constructs to textual FIR. This frontend contains no AVM1 opcode
emitter. Existing FIR operations handle constants/coercion/arithmetic; three new
FIR operations handle dynamic AS globals/member stores. The FIR AVM1 backend emits
target instructions, and the existing SWL linker creates the SWF container.

The IDE embeds the linked SWF and executes its actions in FlashVM for every
complete view. No compiler, parser, Node or Python is needed by the running IDE.
Dynamic Unicode strings and owner-local UI objects are bound in its FlashVM
context; Dis passes checked values, never native heap addresses.

## Implemented source profile

Nonempty sequences of explicit-semicolon assignments are supported:
`object.property = expression;` and `global = expression;`. Expressions support
global reads, nonnegative decimal int32 constants, booleans and `+ - * /` with
ordinary precedence. `+` lowers to dynamic `as.add`; the other operators explicitly
convert operands with `as.to_number` before f64 arithmetic. Values retain AS
semantics, including strings passed to a property. The LHS object is read before
the RHS is evaluated. ASCII source identifiers/whitespace and `//` comments,
including EOF comments, are accepted. Unicode data arrives through runtime values.

Functions/calls, source string literals, nested member reads, parentheses, control
flow, classes and general AS2 lowering remain absent. The shared syntax frontend
can recognize more than this profile; unsupported lowering fails explicitly.
Bounds: source 16 KiB, names 253 bytes, staged FIR 64 KiB. Assignment comments
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

The embedding test checks 600 failure/recovery cycles and atomic output capacity
failure. FIR tests check operation types, effect ordering, canonical roundtrip and
Dis rejection. `disvm_as_view_execution` loads the **linked SWF** into FlashVM,
changes geometry and checks Unicode values across GC. Graphical browser replay
checks actual TextField pixels, row highlighting and native mouse actions. These
are separate from physical desktop/touch acceptance.

General AS AST/FIR lowering, calls/functions, full source maps and migration of
remaining C-rendered editor widgets stay open. Document/browser policy is Limbo;
AS handles the implemented TextField content/geometry; native font/raster/window
and OS ports provide primitives.

The current profile emits one `@ide_view` initializer; combining multiple profile units requires distinct module naming in a later API. / Текущий профиль выдаёт один инициализатор `@ide_view`; выбор разных имён модулей для нескольких единиц — следующий шаг API.
