# Exact int arithmetic and computed Unicode bounds — compiler 0.27

[Русский](INT_ARITHMETIC.ru.md) · [Commented source](../examples/tables/int-arithmetic.b) ·
[Unicode strings](UNICODE_STRINGS.md) · [Compound updates](COMPOUND_UPDATES.md)

The Coco/R big/table profile accepts same-type int `+`, `-`, `*`, `/`, `%` and
unary `+`/`-`. Numeric literals still default to big: write `index + int 1`, not
`index + 1`. Mixed int/big operands fail statically. Table values remain big;
cast an int explicitly before storing it. Int compound `+=`, `-=`, `*=`, `/=`, `%=` forms require same-type RHS int.
Compiler 0.28 adds int bitwise operators; compiler 0.29 adds checked int shifts. [Details](TEXT_BITS.md).

Homogeneous `value(): int` source additionally supports signed division/remainder,
including /= and %=. Real/AS division keeps its previous behavior; real/AS
remainder is still rejected. This does not add general functions, source loops,
implicit numeric coercions or remote atomic compound updates.

```limbo
length: int = len text;
first := length % int 2;
width := length / int 2;
end := first + width;
digits := text[first:end];
return big digits;
```

## Semantics and lowering

Every result is exact signed i32 modulo 2^32, including intermediates. Division
truncates toward zero and remainder has the dividend's sign. MIN/-1 wraps to MIN;
MIN%-1 is zero. Zero divisors raise even if the result is unused. The same rules
hold before/after optimization and in scalar/hosted Dis execution.

Two append-only FIR IDs, `i32.div.signed` and `i32.mod.signed`, consume i32/i32
and return i32. Both are MAY_THROW. Existing add/sub/mul/comparison IDs and all
previous IDs/types remain stable. The peephole folds only known nonzero divisors,
using widened int64 arithmetic so MIN/-1 cannot overflow the C compiler. Zero
constant divisors remain live operations until a genuinely skipped CFG arm is
removed. The independent reference runner implements the same typed arithmetic.

Dis uses existing ADDW/SUBW/MULW/DIVW/MODW. NC64 words are wider than i32, so each
arithmetic result is explicitly normalized before reuse:
`((word & 0xffffffff) ^ 0x80000000) - 0x80000000`.
Both backends apply the same three-step normalization. Normalized input ranges
also make int MIN/-1 safe in native WORD64 division before wrapping its result.
There is no binary64 conversion or new runtime opcode/module helper/container tag.

FIR 0.61 supports live int32 div/mod in SWF with a proven nonzero literal
divisor; unknown/zero divisors still reject without replacing output. See
[exact SWF proof and example](SWF_INT_DIVMOD.md). General AS/SWF dynamic
checked division remains separate work.

## Testing and SDK use

Tests compare signed endpoints and sign combinations using exact Python oracles,
on both homogeneous scalar and runtime table-loaded operands. They cover int
compound updates, unary signs, overflow before reuse, computed Unicode bounds,
unused zero errors, lazy skips, mixed-kind rejection, textual FIR signatures and
unsupported SWF output preservation. Actual VM cases check dispatch counters and
executed captures with one-instruction GC/scheduler slices.

The companion upstream series adds genuine native buffered SEND/RECV copies and
ring commits on AMD64/ARM64. Those commands use a pure C geometry preflight but
perform payload/state publication natively; unsupported/blocking/queued-pointer
cases remain C. Availability is 166/173 commands, not complete semantic coverage.
Our reverse readiness fix prevents --check from creating Python bytecode files.

## Run

After independently building FIR and runtime, from the repository root:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/int-arithmetic.b build/int.fir -g build/int.json
build/fir/fir1 check build/int.fir
build/fir/fir1 opt build/int.fir build/int-opt.fir
build/fir/fir1 dis build/int.fir build/int.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/int.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/int.dis
```

Values: `42, 0`; executed flags: `1, 0`. Relocated SDK acceptance relinks fresh
in-process source+FIR C99 clients and runs original/optimized tutorials in both
VM modes. Physical ARM64 execution and full Inferno boot remain separate acceptance.

Compiler 0.28: [string concatenation/equality and int bit operations](TEXT_BITS.md).

Compiler 0.29: [checked int shifts and compound updates](INT_SHIFTS.md).

Compiler 0.30: [Unicode ordering and checked text/key-to-int casts](TEXT_VALUES.md).
