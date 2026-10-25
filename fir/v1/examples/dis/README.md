# Our Limbo → FIR → Dis tutorial

[Русский](README.ru.md) · [Target/limits/API](../../doc/DIS_BACKEND.md) ·
[Commented source](main.b) · [Equivalent FIR](main.fir) · [C99 library client](main.c).

The example proves a working bounded path through our own Coco/R source
compiler, verifier and C99 Dis emitter. `main.b` starts with INT32_MAX, wraps
addition and multiplication, then subtracts one to produce **2147483647**.
Each intermediate operation wraps even though NC64 stores words in eight bytes.
`main.fir` spells out the same operations and remains usable with the SWO target.

From the repository root, after building FIR and our DisVM separately:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/main.b build/dis-example.fir
build/fir/fir1 dis build/dis-example.fir build/dis-example.dis
build/editor/disvm --jit --stats --inspect-words 0 1 build/dis-example.dis
build/editor/disvm --no-jit --stats --inspect-words 0 1 build/dis-example.dis
```

The `build/fir` and `build/editor` directories are explicit examples; use your
actual tool paths. `DATA 0 2147483647` is the captured first trace, not a static
success message. The compiler does not run the program to obtain that value.
No temporary Inferno compiler or assembler participates in this pipeline.

To demonstrate in-process embedding, run the built `fir1_dis_example` instead:

```sh
build/fir/fir1_dis_example fir/v1/examples/dis/main.b build/dis-example.dis
build/editor/disvm --jit --inspect-words 0 1 build/dis-example.dis
```

`main.c` calls `fir_source` and `fir_run(FIR_DIS)` directly. It owns the temporary
FIR/Dis streams, preserves existing output until compilation succeeds, checks
I/O and closes every opened stream. The exported libraries print nothing and
do not terminate the process. For an IDE, execute this blocking work on its
build worker and send copied results to the channel-owned UI.

This profile has one block, i32 constants/copy/wrap arithmetic and captured
trace/return. It has no imports, exported Limbo `value` ABI, strings, source
channels or Dis SBL. The source tutorial return becomes FIR `@main` trace capture;
ordinary execution without `--inspect-words` does not print it. It is not yet
possible to compile the complete graphical IDE with this subset.
