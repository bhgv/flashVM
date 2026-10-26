# Exact big arithmetic: Coco/R → FIR → Dis

[Русский](BIG_ARITHMETIC.ru.md) · [Commented complete module](../examples/tables/arithmetic.b) ·
[Reflection and Limbo casts](TYPE_REFLECTION.md) · [Extension status](EXTENSIONS.md)

The big-result Limbo profile now supports binary `+`, `-`, `*`, unary `+`/`-`,
parentheses and arithmetic in table keys, reads, writes and slice bounds.
Multiplication binds more tightly than sums; both binary levels associate left.
Casts, typeof, len and keysof take a unary operand, so `string (1 + 2)` formats 3,
while `string 1 + 2` is rejected as string-plus-big. No implicit coercion is added.

All operands must be big. Operations use exact modulo-2^64 bits, matching the
Dis interpreter's unsigned implementation and native AMD64/ARM64 ALU paths.
Examples: MAX+1 is MIN, MIN-1 is MAX, -MIN is MIN, and MAX*2 is -2.
The C compiler/reference runner never uses overflowing signed arithmetic or a
floating-point intermediate. Out-of-range source literals are still rejected;
spell the signed minimum as `-9223372036854775808` or `big -9223372036854775808`.

```limbo
# Body of value(): big; the linked example contains the full module.
maximum := big 9223372036854775807;
wrapped := maximum + 1; # Exact minimum, not a rounded real.
flipped := -wrapped;    # Minimum negation wraps to the same bit pattern.
values := table of big;
key := string flipped; # Exact decimal key copied with Limbo cast syntax.
values[key] = big 7;
values[key] = values[key] + 5 * values[key]; # Evaluate captured reads in order.
return values[key];    # 42 in reference, interpreter and JIT execution.
```

## IR and native code

FIR appends total pure i64 operations `big.add`, `big.sub`, `big.mul` and `big.neg`
without renumbering earlier IDs. Arity, result/operand types and SSA dominance are
verified. The optimizer folds constants with unsigned 64-bit arithmetic and a
portable signed-bit interpretation. It pools identical big constants and performs
same-block CSE only on immutable SSA inputs. Table reads and conversions retain
their conservative effects; eliminating unused arithmetic does not erase a
missing-key or invalid-conversion error in an operand.

The NC64 emitter generates existing IADDL (119), ISUBL (120) and IMULL (123), with
a frame-relative middle operand. Negation emits 0 minus the captured operand.
There is no arithmetic RPC, builtin arithmetic helper or new Dis opcode. Original
and optimized programs execute on the actual Dis VM with quantum=1. Tests decode
the image independently to check native ALU opcodes and absence of MFRAME/MCALL
in the pure arithmetic example. No external assembler or bootstrap is invoked for arithmetic lowering.

## Source positions and bounds

FIRDBG1 records the exact arithmetic/prefix token. Signed literal spans include
the minus and complete digit sequence. Sums/products use loops, so a long flat
expression does not consume recursion budget. Groups, unary operations and nested
indices share the bounded depth of 64; input/SSA/name limits remain in force.
Full source-to-Dis SBL is still separate work.

Shifts, comparisons in this profile, table CFG/loops,
heterogeneous values and automatic channel any/proxy operators remain pending.
An ordinary read-compute-write sequence is not an atomic remote owner update.
The snapshot-key API remains explicit; no pointer crosses heaps or channels.

[Division/remainder and full-width bit operations](BIG_BITWISE.md).

[Commented division tutorial](../examples/tables/division.b) demonstrates runtime MIN/-1, signed remainder, dividend reconstruction and an unselected zero-divisor arm. Both relocated SDKs exercise original/optimized FIR through fresh C99 API clients and both VM modes.
