# Text expressions and exact int bits — compiler 0.28

[Русский](TEXT_BITS.ru.md) · [Commented tutorial](../examples/tables/text-bits.b) ·
[Examples README](../examples/tables/README.md) · [Strings](UNICODE_STRINGS.md) ·
[Int arithmetic](INT_ARITHMETIC.md)

The Coco/R Limbo big/table profile now supports building editor labels and
comparing reflected type names without leaving the source/FIR pipeline:

```limbo
mask := int 255;
mask &= int 63;
mask ^= int 21;
label := "line " + string mask;
old := label;
label += " 😀";
valid := (old == "line 42") && (label != old);
```

## Typed source contract

| Inputs | Operators | Result |
| --- | --- | --- |
| string, string | `+`, `+=` on a string local | string |
| string, string | `==`, `!=` | bool |
| int, int | `&`, `|`, `^`, `&=`, `|=`, `^=` | int |
| int | `~` | int |

Operands evaluate once, left to right. Compound updates capture the old value,
evaluate the RHS, then rebind the local after success. They are not remote atomic
transactions. Numeric/text or int/big mixtures require explicit conversions.
Compiler 0.30 adds string ordering; table equality remains rejected. Compiler 0.29 adds checked int shifts;
the existing checked big shifts are unchanged. This does not extend the separate
homogeneous int or AS numeric source profiles. AS tobig/tostring extensions retain
their own syntax; the examples above use Limbo casts.

String equality compares Unicode scalar contents, not addresses. There is no
normalization, case folding or grapheme segmentation: `"é" != "é"`.
Concatenation preserves order, accepts empty operands and does not mutate either
source. Existing aliases and half-open slices retain their original values after
`+=`. A concatenated result can be a table text key or feed len/index/slicing.
`typeof value == "big"` now works directly for supported reflected values.

## FIR, optimizer and VM

Two append-only operations preserve all previous FIR IDs:

| FIR | Signature | Effects |
| --- | --- | --- |
| `string.concat` | string/string → string | reads memory, allocates, may throw |
| `string.eq` | string/string → bool | reads memory |

The verifier checks both operands and the result through textual and public C99
IR APIs. Unused concatenation remains live after optimization because allocation
failure is observable. Constant string equality folds to bool; dynamic equality
continues to read its operands. The reference runner copies exact byte extents,
including non-terminated slices, and registers concatenation owners separately
from aliases so cleanup frees each allocation once.

Hosted NC64 Dis emits existing ADDC and BEQC. Pointer operands/results are present
in frame root maps across allocation and one-instruction GC/scheduler quanta.
This uses existing AMD64/ARM64 interpreter/JIT paths and their C fallbacks; no
new Dis opcode, channel protocol or builtin module ABI is introduced.

Int bit operations widen each operand exactly to big, reuse full-width pure
big.and/or/xor/not, then narrow modulo 2^32. The result remains signed int:
`~int 0` is -1, and the sign bit survives cast/compound updates. This avoids
floating conversion and reuses the established optimizer and Dis native kernels.
No table owner RPC is added for these scalar operations.

Live string.concat/string.eq currently require Dis or reference execution; the
SWF object backend rejects unsupported live operations before replacing output.
This feature prepares IDE source services; it does not connect IDE Build/Run/Debug
panels or complete debugger attachment.

## Run and expected result

From the repository root after building FIR and the native runtime:

```sh
mkdir -p build/tmp
export TMPDIR="$PWD/build/tmp"
build/fir/fir1_source limbo --input fir/v1/examples/tables/text-bits.b build/text.fir
build/fir/fir1 check build/text.fir
build/fir/fir1 opt build/text.fir build/text-opt.fir
build/fir/fir1 dis build/text-opt.fir build/text.dis
build/runtime/disvm --jit --quantum 1 --inspect-words 0 4 build/text.dis
build/runtime/disvm --no-jit --quantum 1 --inspect-words 0 4 build/text.dis
```

Expected trace values: 42, 0. Execution flags: 1, 0. Installed SDK acceptance also
relinks fresh in-process source/FIR C99 clients, compiles the original/optimized
tutorial and checks both VM modes. ARM64 hardware acceptance is separate from
existing instruction-emulation checks.

Compiler 0.29: [checked int shifts and compound updates](INT_SHIFTS.md).

Compiler 0.30: [Unicode ordering and checked text/key-to-int casts](TEXT_VALUES.md).
