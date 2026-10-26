# Full-width big arithmetic in direct Dis control flow — FIR 0.17

[Русский](BIG_CFG.ru.md) · [Complete commented FIR](../examples/dis/big-cfg.fir) ·
[Scalar target](DIS_BACKEND.md) · [Arithmetic contract](BIG_BITWISE.md)

The direct scalar backend now accepts i64 constants/copies/traces and big
add/subtract/multiply/divide/remainder/and/or/xor/negate/complement in verified
multi-block FIR. Previously every big instruction selected the single-block
hosted table backend, so valid big CFG was rejected and pure big arithmetic
unnecessarily loaded Bigtable. Dispatch now selects the table backend only for
actual table, key, range, reflection or conversion operations.

The source grammar is unchanged: this is a FIR/backend capability. The big-result
Coco/R Limbo source profile still admits a single-block body; this does not add
source if/for/while, big comparisons, phi values, mixed-type arithmetic, shifts,
transparent any/proxy access or full Limbo function/module lowering.

## Binary and ownership contract

The scalar image uses existing MOVL/ADDL/SUBL/DIVL/MODL/MULL/ANDL/ORL/XORL
instructions (118–126), with no imports or heap references. Negation subtracts
from the initialized zero scratch cell. Complement XORs with a full-width
all-ones value in the existing per-result module-data cell. No new global mask,
runtime helper, opcode or cross-heap pointer is introduced.

Instruction counts include every expansion before branch targets are resolved.
Forward/backward branches carry instruction PCs, not byte offsets or FIR indices.
Big arithmetic remains modulo 2^64; division/remainder retain checked zero errors
and the MIN/-1 contract. Unused division can still throw after optimization.
Only an unreachable failing arm may disappear with CFG simplification.

The capture layout remains N eight-byte values followed by N executed flags.
Unexecuted captures stay zero with flag zero. Count the live traces again after
optimization: dead-arm traces and their slots can disappear. Both descriptors
have empty pointer maps. Actual tables retain the separate hosted path with
signature-checked imports and precise owner maps; multi-block table CFG remains
unsupported. Do not treat ordinary arithmetic as an atomic remote mutation.

## Run the example

From the repository root, with our FIR compiler and DisVM already built:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1 dis fir/v1/examples/dis/big-cfg.fir build/big-cfg.dis
build/runtime/disvm --jit --inspect-words 0 4 build/big-cfg.dis
build/runtime/disvm --no-jit --inspect-words 0 4 build/big-cfg.dis
```

Expected cells: `-9007199254740993, 0, 1, 0`. The first arm reconstructed the
original number; the second arm did not execute. Use `fir1 opt INPUT OUTPUT`
explicitly to see the constant decision and unreachable arm disappear.
The example is also installed with the compiler SDK.

## Verification

The runtime acceptance driver tests both decisions, all ten operators, signed
extrema, all quotient/remainder sign combinations, values above 2^53 and seeded
independent Python oracles. Entry-defined big values are used across blocks;
arm exits jump backward. Original and optimized images execute with quantum one
in interpreter/JIT modes. Decoding checks actual native opcode availability,
zero flags/imports and empty GC maps. Selected failing arms reject inspection
without publishing DATA; unselected failing arms succeed before/after optimization.
These tests exercise AMD64 runtime execution. ARM64 emitter instruction-emulation
coverage is separate and does not establish physical ARM64 execution.
