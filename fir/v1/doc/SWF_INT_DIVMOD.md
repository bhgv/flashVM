# Signed int32 division and remainder in SWF — FIR 0.61

[Русский](SWF_INT_DIVMOD.ru.md) · [Commented source](../examples/dis/int-divmod-swf.b) ·
[C99 API regression](../tests/swf_int_divmod_api.c) ·
[General int arithmetic](INT_ARITHMETIC.md) · [Literal-count shifts](SWF_INT_SHIFTS.md)

The SWO backend now accepts `i32.div.signed` and `i32.mod.signed` when the
right operand's live SSA definition is a **nonzero const.i32**. The numerator
may be dynamic. Unknown divisors and literal zero reject before any object
bytes are written. CLI, `fir_object` and `fir_run(FIR_OBJECT)` share this check;
explicit optimization may expose a literal through a copy or constant expression.
No source grammar, FIR opcode, SWO version or active Dis ABI changes.

| Input | Quotient | Remainder |
| --- | ---: | ---: |
| -7 and 3 | -2 | -1 |
| 7 and -3 | -2 | 1 |
| MIN and -1 | MIN, wrapping | 0 |
| any numerator and unknown divisor | SWO rejection | SWO rejection |
| any numerator and zero | SWO rejection | SWO rejection |

Remainder follows the dividend sign, not Python's floor-modulo convention.
The mathematical quotient truncates toward zero. MIN/-1 wraps to MIN by the
FIR contract. The AVM1 emitter performs Divide or Modulo, then BitOr with zero
to obtain exact int32 and canonicalize a negative-zero remainder to integer zero.
The linker admits standard one-byte Modulo 0x3f, preserving exact action bytes
and its framing/branch checks. No extension flag or custom player opcode is needed.

## Why double division is sufficient here

Every int32 input is exactly representable as binary64. For nonzero integer
b, a nonintegral exact quotient a/b is at least 1/abs(b) from an integer.
Since abs(a)<=2^31, binary64 rounding error is much smaller than that distance:
less than 2^-21/abs(b), even allowing a full rounding ulp. Thus rounding cannot
cross a truncation boundary. Integral quotients, including positive 2^31 for
MIN/-1, are exactly representable. ToInt32 then implements truncation and wrap.
This argument is specific to signed32 operands; it does not justify big lowering.
Modulo operates on the exact integer inputs, followed by the same normalization.

## Unused checked arithmetic

The optimizer may remove an unused i32/big division or remainder after proving
a same-type literal divisor is nonzero. It may also remove an unused checked
shift after proving an int32 literal count in 0…31 or 0…63 for big. This is a
local DCE proof, not a change to the public conservative `fir_effects` table.

Each operand producer retains its own effects. An invalid conversion, missing
table key or inner zero divide remains observable even when a total outer
operation has no users. Unknown/invalid divisors and counts retain their checks.
No load, allocation, ownership, suspension or control effect is weakened. CSE
and memory barriers keep their existing conservative treatment.

## Build and run

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/dis/int-divmod-swf.b build/divmod.fir
build/fir/fir1 object build/divmod.fir build/divmod.swo
build/linker/swl link build/divmod.swf build/divmod.swo
build/fir/fir1 dis build/divmod.fir build/divmod.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 build/divmod.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 build/divmod.dis
```

The fully commented source visits -7…7, reconstructs each i as `(i/3)*3+i%3`,
sums to zero and returns **42**. Dis reports payload 42 and flag 1. The linked
SWF traces 42 in FlashVM. Both raw and optimized FIR are supported; use `fir1
opt` explicitly for the optimized variant. The installed SDK includes this
source, public libraries and guides; IDE builds use the same in-process APIs.

The earlier integer-identities tutorial can now emit raw SWO too: its divisors
±1 and zero shift counts satisfy the proofs. Optimization still removes those
calculations; target admission no longer requires removing them first.

## Evidence and remaining limits

The cross-target regression compares 2688 raw/optimized values from 32 numerator
inputs and 21 divisors, including MIN/MAX, both signs, exact/fractional quotients
and deterministic full-width values. A mathematical Python integer oracle avoids
host divide overflow and float rounding. Actual FlashVM SWF output and native
Dis interpreter/JIT captures must agree; dispatch counters and execution flags
are checked. Zero/unknown divisors check eager errors and preservation of old
output. C99 API tests repeat success, rejection, copy-proof and optimizer calls.
Separate DCE fixtures cover total/invalid/unknown int32 and big operations and
preserved producer traps. Linker tests exercise object/archive byte preservation
and malformed framing rejection.

Actual SWF execution requires explicit `FIR_SWL` and `FIR_FLASHVM_RUNNER` test
inputs. Compiler-only tests do not establish player execution. General runtime
checked division by an unknown value in SWF, arbitrary big SWF arithmetic,
AS3/ABC output and executable new Dis64 modules remain separate work.
