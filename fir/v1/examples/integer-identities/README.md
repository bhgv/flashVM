# Integer identities across Dis and SWF

[Русский](README.ru.md) · [Every-line-commented Limbo](main.b) ·
[Exact rules and limits](../../doc/INTEGER_IDENTITIES.md)

The accumulator adds three copies of each loop index 0, 1, 2, 3, then 24:
3×6+24 = **42**. Multiplication by zero, remainder by one and self-XOR contribute
zero. Division by one, shift by zero and AND with -1 retain the index. These are
integer identities, not floating-point rules. The dynamic loop exposes the new
peephole rather than relying on an entirely constant expression.

Build our FIR C99 tools with the selected Coco/R SDK, the SWL linker, and the
matched DisVM runtime. No legacy Limbo/bootstrap is used to compile this source.
From the repository root:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/integer-identities/main.b build/identities.fir -g build/identities.json
build/fir/fir1 opt build/identities.fir build/identities.opt.fir
build/fir/fir1 dis build/identities.opt.fir build/identities.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/identities.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/identities.dis
build/fir/fir1 object build/identities.opt.fir build/identities.swo
build/linker/swl link build/identities.swf build/identities.swo
```

Dis reports DATA 0 42 and DATA 8 1, the result and executed flag. An actual
FlashVM host such as [run_flashvm.c](../../tests/run_flashvm.c) reports 42 for
the linked SWF; [build instructions](../../doc/TESTING.md) explain its explicit
static dependencies. Raw FIR also executes as Dis, but raw SWO emission rejects
the retained general checked operators without replacing existing output.

For embedding, `fir_source_debug` produces FIR plus an optional source sidecar;
`fir_run` with optimise=1 emits FIR_OBJECT or FIR_DIS through the same pass.
[Executable C99 client regression](../../tests/integer_identities_api.c) shows
stream cleanup, snapshot borrowing and repeated recovery after raw rejection.
The caller owns all streams. FIR module/snapshots must not escape their calls.
There is no deployed Python requirement and no change to either VM heap or ABI.

## Runnable library client

[main.c](main.c) explains every code line. The regular FIR build produces
`build/fir/fir1_integer_identities_example`; an installed SDK ships that program,
its source and the matching static libraries. Run:

```sh
build/fir/fir1_integer_identities_example fir/v1/examples/integer-identities/main.b build/identities-api.dis dis
build/fir/fir1_integer_identities_example fir/v1/examples/integer-identities/main.b build/identities-api.swo object
```

The client stages complete compiler output before opening its destination.
Final filesystem write failures can leave a prefix; no atomic rename is claimed.
Relocated SDK tests recompile this client with installed headers/libraries,
compare both outputs byte-for-byte with CLI optimization and execute its Dis
through the interpreter and JIT. The SWO output still needs the SWL link step.

To match CLI bytes, this client explicitly materializes canonical optimized FIR
between FIR_OPT and backend calls. Direct in-memory optimization preserves old
SSA identities and can reserve different slots; see the [identity contract](../../doc/INTEGER_IDENTITIES.md).

## Comparison example

[comparisons.b](comparisons.b) adds total integer comparison proofs to the
same source/compiler pipeline. Its loop sums 0+1+2+3 and returns that sum+36.
The `i == i` branch is selected; `i < i`, `i < MIN` and `MAX < i` branches are
impossible. Optimization removes those predicates and their unreachable returns
while retaining the loop test. Use this input in place of main.b in the CLI
commands above; both raw and optimized object targets support this example.
The line comments explain every statement. No extra library or ABI is needed.
