# Paired integer cancellation and bit absorption — FIR 0.62

[Русский](INTEGER_PAIRS.ru.md) · [Commented source](../examples/dis/integer-pairs.b) ·
[Regression](../tests/integer_pairs.py) · [Existing identities](INTEGER_IDENTITIES.md)

The verified peephole replaces paired i32/big expressions using modular integer
identities. No opcode, parser production, ABI or backend format is added. CLI
and embedded `fir_optimise`/`fir_run` use the same implementation.

| Pattern | Result |
| --- | --- |
| (x+y)-y, (y+x)-y | x |
| (x-y)+y, y+(x-y) | x |
| (x XOR y) XOR y and operand-swapped forms | x |
| x AND (x OR y), (x OR y) AND x | x |
| x OR (x AND y), (x AND y) OR x | x |
| x AND (x AND y), (x AND y) AND x | x AND y |
| x OR (x OR y), (x OR y) OR x | x OR y |

Addition/subtraction is modulo 2^32 or 2^64, so cancellation remains exact at
MIN/MAX and across overflow. Equality here means the same immutable SSA value,
or exact same-width integer literals under separate definitions. It does not
mean equal names, equivalent source text, similar floating values or a guessed
runtime range. Different third operands are not canceled.

A dominating pure definition may be in an earlier block. The rewrite changes
the outer result to a copy; it moves or reevaluates no producer and derives no
new memory facts. Existing copy propagation and DCE retire dead pure operators.
The checked producer of an operand retains its own effects. For example, a
zero divide producing y must still fail after `(x XOR y) XOR y` becomes x.
Memory/ownership/suspension policies and ordered local CSE remain conservative.

These rules never apply to binary64 or dynamic AS coercions. The counterexample
`(1+1e300)-1e300` yields zero, not one; the regression executes both variants.
Source/debug sidecars continue to describe original source/FIR identities;
this does not introduce final Dis-PC/SBL mapping or a new debugger protocol.

## Compile and execute

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/integer-pairs.b build/pairs.fir
build/fir/fir1 opt build/pairs.fir build/pairs.opt.fir
build/fir/fir1 dis build/pairs.opt.fir build/pairs.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/pairs.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/pairs.dis
build/fir/fir1 object build/pairs.opt.fir build/pairs.swo
build/linker/swl link build/pairs.swf build/pairs.swo
```

The fully commented source loops over i=0…3, restores three copies of i through
add/sub cancellation, repeated XOR and absorption, sums eighteen and adds24.
Raw and optimized Dis/SWF return42; Dis capture reports value42 and flag1.
Raw SWF remains supported because every operation already has ordinary AVM1
lowering. Optimization removes calculations rather than enabling a new format.

The installed SDK ships this source. Its existing C99 clients accept the input
and use the same libraries; the relocated acceptance rebuilds clients and
compares explicitly matching CLI/API pipelines before execution. The Dis-owned
IDE compiler-worker fixture also includes paired expressions in its dynamic
loop and compares all24 language/optimization/target artifacts with the CLI.

## Evidence

The matrix covers16 operand-order forms ×5 boundary input pairs ×2 widths:
160 cases, raw and optimized. Additional checks cover independent equal literals,
a cross-block dominating definition, different third operands, preserved producer
errors, the IEEE counterexample, stable reoptimization and real source lowering.
Actual Dis tests check payloads, execution flags, owner cleanup and native/JIT
counters. Actual SWF tests require explicit FIR_SWL/FIR_FLASHVM_RUNNER inputs.
Compiler-only acceptance does not establish player execution.

No full Limbo/AS3 compiler, new executable Dis64 ABI, ref-result GC/unwind or
physical ARM64 acceptance is implied by this optimizer change.
