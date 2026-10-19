# AS3: source → FIR → SWF

[Русский](README.ru.md). Teaching subset: [coverage](../../doc/COVERAGE.md).

`main.as` is the source; `expected.fir` is readable expected IR;
`expected.txt` contains expected trace lines. Result: **5.75**.

All operands are numeric literals, so the adapter can select f64 operations.
Variables, strings, classes and dynamic source-language addition are rejected.
The AS3 sample uses an explicitly limited numeric-to-AVM1 profile; ABC/AVM2
emission is not implemented.

From the repository root, after [building](../../README.md):

```sh
build/fir1/fir1_source as3 --input fir/v1/examples/as3/main.as build/fir1/as3.fir
build/fir1/fir1 check build/fir1/as3.fir
build/fir1/fir1 opt build/fir1/as3.fir build/fir1/as3.opt.fir
build/fir1/fir1 swf build/fir1/as3.opt.fir build/fir1/as3.swf
```

Example API: `trace(value)` prints the computed number; `value` is its single
argument. [FIR operations](../../doc/FORMAT.md), [CLI](../../compiler/README.md),
[optimisation](../../optimiser/README.md), [execution checks](../../doc/TESTING.md).
Local viewer: `python fir/v1/view_docs.py`.
