# Checked signed32 shifts — compiler 0.29

[Русский](INT_SHIFTS.ru.md) · [Commented tutorial](../examples/tables/int-shifts.b) ·
[Examples](../examples/tables/README.md) · [Int arithmetic](INT_ARITHMETIC.md) ·
[Text and bit expressions](TEXT_BITS.md) · [Big shifts](BIG_SHIFTS.md)

Our Coco/R Limbo big/table profile now accepts `<<`, `>>`, `<<=` and `>>=` with
an int value and int count. The result remains int. Big shifts keep their existing
big/int contract and 0..63 count range; int shifts require **0..31**.

```limbo
packed := int packet.word;
sign := packed >> int 31;
value := packed & int 255;
value <<= int 1;
value >>= int 1;
return big value;
```

## Types, width and errors

Both operands evaluate once, left to right. The count is not masked: negative
counts and 32 or larger raise `invalid argument`, including when the value is
zero or the result is unused. Explicit int casts retain the existing modulo-2^32
conversion rule; count validation applies to the resulting typed int value.

Left shift keeps the low 32 bits, then interprets them as signed. Right shift is
arithmetic, propagating the sign bit. Shift by zero preserves the input;
`int 1 << int 31` yields -2147483648 and `int -1 >> int 31` yields -1. No signed
C left shift or negative signed C right shift is needed by the compiler/reference
oracle. There is no conversion through binary64 or implicit int/big mixing.

Compound updates capture the original scalar before RHS evaluation and publish
the new local binding only after success. Remote/channel atomic transactions are
not added. Sum binds tighter than shift, and shifts associate left to right, as
in the existing big grammar. The separate homogeneous int and AS numeric source
profiles are unchanged; this extends our typed big/table profile.

## FIR and optimizer

Two append-only FIR IDs preserve every earlier operation/type ID:

| Operation | Signature | Effect |
| --- | --- | --- |
| `i32.shl.checked` | i32, i32 → i32 | may throw |
| `i32.shr.checked` | i32, i32 → i32 | may throw |

Textual FIR and caller-constructed C99 IR both enforce the exact signature.
Constant folding accepts only counts 0..31, uses unsigned32 bits and explicitly
reconstructs the sign. Invalid constant shifts stay live even when unused.
Genuinely unselected CFG arms may be removed without evaluating their shifts.
The source debug JSON retains the exact operator span after UTF-8 prefixes.

## NC64 Dis execution

Both scalar and hosted Dis emitters expand one shift into eight existing Dis
instructions: two count guards, a big-width shift, three signed32 normalization
steps, a jump over the error edge and the error edge itself. Valid normalized
inputs use existing SHLL/SHRL kernels with counts 0..31. Invalid counts execute
an existing checked big shift with count 64, which raises the VM's normal
invalid-argument exception. This requires no new opcode, builtin helper ABI or
module dependency. The error edge never publishes a successful result.

The guards account for Dis branches comparing source against middle, while
arithmetic shifts use middle as the value and source as count. Scalar sizing and
hosted measure/write passes both include all eight instructions, preserving
branch targets after the expanded operation. Existing AMD64/ARM64 kernels and
C fallbacks remain authoritative. This series introduces no new native fast path.

## Validation and limits

Tests cover constants and table-loaded values at signed endpoints, counts
0/1/7/16/30/31, arithmetic sign propagation, repeated compound updates and invalid
counts -1/32/63/64/INT_MIN/INT_MAX with zero/positive/negative values. Raw scalar
FIR adds branching trace captures after shift expansion. Original/optimized
reference and actual interpreter/JIT execution are compared with Python integer
oracles; forged C99 types fail before either backend writes output.

Literal-count checked int shifts (0…31) are supported by the SWF object backend;
unknown counts still reject. See [exact SWF admission and example](SWF_INT_SHIFTS.md).
General cross-module functions and full source Limbo remain separate work.
Language changes stay in FlashVM; the Inferno snapshot is not modified for these
compiler operations.

## Run

From the repository root after building FIR and runtime:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/int-shifts.b build/shifts.fir -g build/shifts.json
build/fir/fir1 check build/shifts.fir
build/fir/fir1 opt build/shifts.fir build/shifts-opt.fir
build/fir/fir1 dis build/shifts-opt.fir build/shifts.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/shifts.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/shifts.dis
```

Expected trace values: 42, 0; executed flags: 1, 0. Relocated SDK acceptance
relinks fresh source/FIR C99 clients and runs this tutorial before/after
optimization in both VM modes. Physical ARM64 acceptance is separate from
instruction emulation of the existing native kernels.
