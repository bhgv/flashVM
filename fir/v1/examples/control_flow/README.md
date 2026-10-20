# Control Flow

[Русский](README.ru.md).

`if %less, ^yes, ^no` selects the first target for true, second for false.
`jump ^done` transfers control unconditionally. A value defined in one arm
cannot be used in the other: SSA dominance rejects this. Block parameters and
phi are not implemented. `trace` consumes one value and prints its string form.

```sh
build/fir1/fir1 check fir/v1/examples/control_flow/main.fir
build/fir1/fir1 opt fir/v1/examples/control_flow/main.fir build/fir1/control_flow.fir
build/fir1/fir1 object build/fir1/control_flow.fir build/fir1/control_flow.swo
build/linker/swl link build/fir1/control_flow.swf build/fir1/control_flow.swo
```

Expected trace: `yes / да`.
[CLI](../../compiler/README.md), [FIR](../../doc/FORMAT.md),
[tests](../../doc/TESTING.md). `python fir/v1/view_docs.py`.
