# LIMBO: source → FIR → SWF

[Русский](README.ru.md). Teaching subset: [coverage](../../doc/COVERAGE.md).

`main.b` is the source; `expected.fir` is readable expected IR;
`expected.txt` contains expected trace lines. Result: **-2147483648**.

Limbo exports `value(): int`. The adapter inlines its expression into `@main`
and adds trace as an explicit test harness, not an Inferno module loader ABI.

From the repository root, after [building](../../README.md):

```sh
build/fir1/fir1_source limbo --input fir/v1/examples/limbo/main.b build/fir1/limbo.fir
build/fir1/fir1 check build/fir1/limbo.fir
build/fir1/fir1 opt build/fir1/limbo.fir build/fir1/limbo.opt.fir
build/fir1/fir1 object build/fir1/limbo.opt.fir build/fir1/limbo.swo
build/linker/swl link build/fir1/limbo.swf build/fir1/limbo.swo
```

Example API: `trace(value)` prints the computed number; `value` is its single
argument. [FIR operations](../../doc/FORMAT.md), [CLI](../../compiler/README.md),
[optimisation](../../optimiser/README.md), [execution checks](../../doc/TESTING.md).
Local viewer: `python fir/v1/view_docs.py`.

`variables.b` demonstrates initialized declarations and reassignment with fresh SSA values. The tutorial entry prints 36.

## Extended numeric example and optimizer

[syntax.b](syntax.b) comments each computation; syntax.txt records the expected trace.
variables.as/variables.b demonstrates mutable bindings; variables.fir shows SSA.
expected.fir is readable FIR input, not a binary object. Edit FIR and reverify it
before compiling; no silent optimizer pass is implied.

```sh
build/native-fir/fir1_source limbo --input fir/v1/examples/limbo/syntax.b build/native-fir/demo.fir
build/native-fir/fir1 check build/native-fir/demo.fir
build/native-fir/fir1_opt build/native-fir/demo.fir build/native-fir/demo.opt.fir
build/native-fir/fir1 object build/native-fir/demo.opt.fir build/native-fir/demo.swo
build/native-linker/swl link build/native-fir/demo.swf build/native-fir/demo.swo
```

C API streams/buffers belong to the caller; FIR contains no VM structure pointers.
Serialize parser calls; snapshot callbacks borrow IR until return. Optimization
preserves trace/channel effects and source profiles; f64 is not rewritten with
fast-math. AS3 does not emit ABC; Limbo lowers only i32 value(), without a general
module/channel ABI. Cross-built compilers execute on their target machine.
[Native/cross builds](../../../../BUILDING.md),
[C tutorial combining all stages](../../../../linker/examples/ide/README.md).
