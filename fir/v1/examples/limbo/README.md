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
build/fir1/fir1 swf build/fir1/limbo.opt.fir build/fir1/limbo.swf
```

Example API: `trace(value)` prints the computed number; `value` is its single
argument. [FIR operations](../../doc/FORMAT.md), [CLI](../../compiler/README.md),
[optimisation](../../optimiser/README.md), [execution checks](../../doc/TESTING.md).
Local viewer: `python fir/v1/view_docs.py`.

`variables.b` demonstrates initialized declarations and reassignment with fresh SSA values. The tutorial entry prints 36.
