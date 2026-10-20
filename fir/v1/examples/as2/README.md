# AS2: source → FIR → SWF

[Русский](README.ru.md). Teaching subset: [coverage](../../doc/COVERAGE.md).

`main.as` is the source; `expected.fir` is readable expected IR;
`expected.txt` contains expected trace lines. Result: **42**.

All operands are numeric literals, so the adapter can select f64 operations.
Strings, classes and dynamic source-language addition are rejected.
The AS3 sample uses an explicitly limited numeric-to-AVM1 profile; ABC/AVM2
emission is not implemented.

From the repository root, after [building](../../README.md):

```sh
build/fir1/fir1_source as2 --input fir/v1/examples/as2/main.as build/fir1/as2.fir
build/fir1/fir1 check build/fir1/as2.fir
build/fir1/fir1 opt build/fir1/as2.fir build/fir1/as2.opt.fir
build/fir1/fir1 object build/fir1/as2.opt.fir build/fir1/as2.swo
build/linker/swl link build/fir1/as2.swf build/fir1/as2.swo
```

Example API: `trace(value)` prints the computed number; `value` is its single
argument. [FIR operations](../../doc/FORMAT.md), [CLI](../../compiler/README.md),
[optimisation](../../optimiser/README.md), [execution checks](../../doc/TESTING.md).
Local viewer: `python fir/v1/view_docs.py`.

`variables.as` demonstrates initialized declarations and reassignment with fresh SSA values. Output: 36, then 42.
