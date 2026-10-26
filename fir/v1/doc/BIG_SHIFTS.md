# Typed int casts and checked big shifts — FIR 0.20 / runtime 0.23

[Русский](BIG_SHIFTS.ru.md) · [Complete commented Limbo](../examples/tables/shifts.b) ·
[Big arithmetic](BIG_ARITHMETIC.md) · [Acyclic owners](TABLE_CFG.md)

Our Coco/R big-result source profile now supports `<<`, `>>`, explicit `int expr`
and int locals used as counts. Big values remain exact signed 64-bit values;
int retains this compiler's established fixed i32 contract, even though NC64
stores native words in eight-byte cells. This is not a change to Inferno's native
WORD ABI or a claim of full target-dependent Limbo int semantics.

Bare numeric literals remain big in this bounded profile. Write `a << int 3`,
or declare `n: int = int 3;` and use `a << n`. A big count without an explicit
conversion is rejected. Other arithmetic operators still require big operands:
use `int (1 + 2)` to compute a count, rather than mixed `int 1 + 2`.

## Exact semantics

`int expr` admits int or big, keeps the low 32 bits and interprets them as signed.
For example, `int 4294967295` is -1 and `int 4294967296` is 0. It is a total,
effect-free conversion. `big` explicitly promotes an int's signed value; string
conversion formats that value and `typeof` returns `int`. Table elements and
numeric keys remain big: promote explicitly before storing/indexing with int.
No text/real/key-to-int parsing or implicit mixed arithmetic is introduced.

Shift counts must be in 0..63 **after** the explicit conversion. Invalid counts
raise `invalid argument`; they are not masked modulo 64. This includes unused
results after optimization. Left shifts operate on unsigned 64-bit bits and wrap;
right shifts are arithmetic and propagate the sign. Count zero preserves the
input. MIN >> int 63 is -1, and 1 << int 63 is MIN.

Precedence matches the reference Limbo grammar: unary/casts, products, sums,
shifts, bitwise AND, XOR, OR. Shifts associate left. Groups override precedence.
Terminal comparisons continue to operate on complete big expressions. Each
operator's debug origin is its authored two-byte token; int casts retain the
three-byte prefix span.

## FIR, optimization and native code

Three operation IDs are appended without renumbering: `value.int` consumes
i32/i64 and returns i32; `big.shl`/`big.shr` consume i64/i32 and return i64.
Verification enforces types, arity and dominance. Int casts are pure. Shifts
carry MAY_THROW, so an invalid unused operation cannot disappear or be CSE'd.
Constant shifts fold only for known valid counts; arithmetic right folding and
the C reference runner use unsigned bits plus explicit sign fill, with no shift
by 64 or signed-left-shift undefined behavior.

Both direct scalar CFG and hosted acyclic table CFG emit existing SHLL/SHRL
(127/128). Native int normalization uses low-bit AND, sign-bit XOR and subtraction
with ordinary unmapped constants. Scalar copies never use MOVP or GC pointer
maps. There are no new Dis opcodes or runtime arithmetic helpers.

Hosted reflection appends kind 4 = int; kinds 0..3 keep their meanings. This
requires the matching Bigtable implementation for int reflection; an older
module rejects its unknown tag. Function signatures and key namespaces are
unchanged. Validated pairing is compiler 0.20/runtime 0.23. The int extension
stays in FlashVM and is not exported to Inferno.

## Run and verify

From the repository root after independent compiler/runtime builds:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/shifts.b build/shifts.fir -g build/shifts.json
build/fir/fir1_table_run build/shifts.fir
build/fir/fir1 dis build/shifts.fir build/shifts.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/shifts.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/shifts.dis
```

Reference prints 42; native cells are `42, 0, 1, 0`. The second static return is
unselected. Both relocated SDKs relink C99 clients and execute this tutorial.
Tests cover every count 0..63 with runtime-loaded signed extrema, values above
2^53, int copies, native opcode decoding, precedence, casts/reflection, invalid
counts before/after opt, wrong public FIR types and preserved output on rejection.
UBSan checks compiler/folding/reference behavior. Native AMD64 execution and
ARM64 instruction emulation are separate from physical ARM64 acceptance.
