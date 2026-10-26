# Big division, remainder and full-width bit operations

[Русский](BIG_BITWISE.ru.md) · [Complete commented module](../examples/tables/bitwise.b) ·
[Arithmetic foundation](BIG_ARITHMETIC.md) · [Extension matrix](EXTENSIONS.md)

Our Coco/R Limbo big profile supports `/`, `%`, `&`, `|`, `^` and unary `~`.
All operands/results are big; no implicit int, real or string coercion occurs.
The supported precedence agrees with the reference Limbo grammar: unary signs,
casts/reflection and complement bind first, then `* / %`, then `+ -`, then `&`,
then `^`, then `|`. Binary levels associate left. Groups override precedence.

## Division contract

Quotients truncate toward zero. Remainders have the dividend's sign (or zero):
-7/3 is -2, -7%3 is -1, and 7%-3 is 1. Division/remainder by zero raises the same
`zero divide` error as Dis, including unused expressions after optimization.
MIN/-1 returns MIN and MIN%-1 returns zero, matching the existing interpreter.
The compiler and C reference runner guard these cases before using a host / or %;
there is no undefined signed overflow or host division trap in constant folding.

## Bit contract

AND/OR/XOR/complement operate on all 64 bits. Results use the signed big view of
those exact bits: ~MAX is MIN, ~MIN is MAX, and ~-1 is zero. The implementation
uses unsigned bits and a portable signed interpretation. It does not narrow to
AS/int-width operations or pass through a floating-point value.

```limbo
# Body of value(): big; the linked module adds the complete interface and comments.
values := table of big;
values.input = big -123456789012345;
a := values.input;           # A runtime read prevents constant-only execution.
q := a / 97;                # Truncate toward zero.
r := a % 97;                # Same sign as a.
restored := q * 97 + r;
values[string restored] = big 42;
toggled := restored ^ big -1;
back := ~toggled;            # Recover the complete signed bit pattern.
return values[string ((back & big -1) | 0)]; # Exactly 42.
```

## FIR, optimizer and native Dis

FIR appends big.div/mod/and/or/xor/not without renumbering existing IDs. Verification
checks arity, types and dominance. Bit operations are total/pure; division and
remainder carry MAY_THROW and are not removed or CSE'd as pure operations.
Known nonzero constant divisors can fold, with explicit MIN/-1 handling. Zero
constant divisors remain executable failing instructions. Operand errors and
source evaluation order are preserved; staged traces stay unpublished on failure.

NC64 lowering emits existing DIVL (121), MODL (122), ANDL (124), ORL (125), XORL
(126). Complement is XOR with a plain full-width all-ones module-data cell, not a
heap owner or wire pointer. There are no arithmetic helper calls. The guarded JIT
may retain checked C handling for exceptional inputs; both dispatch modes have
the same language results/errors. Validated SDK pairing: compiler 0.16 and runtime 0.19.

The test suite checks decoded opcodes, independent Python signed/modulo oracles,
all sign combinations, extrema, values above 2^53, dynamic table reads, exact
operator source spans, unused errors and per-instruction GC before/after optimization.
The source compiler/optimizer/reference suite also runs under UBSan.

Shifts, mixed int/big typing, profile comparisons, table CFG/loops, general any and
transparent proxy operations remain pending. These operators do not make a table
read-compute-write sequence an atomic owner RPC. No extension is exported to Inferno.
