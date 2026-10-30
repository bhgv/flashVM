# Typed integer conversion facts — FIR0.69

[Русский](CAST_FACTS.ru.md) · [Analysis](../optimiser/integer_facts.c) ·
[C99 oracle](../tests/integer_cast_facts_api.c) ·
[Target matrix](../tests/integer_cast_facts.py) ·
[Commented Limbo example](../examples/dis/cast-facts.b)

The shared CLI/library optimizer now carries known bits across existing typed
`value.big` and `value.int` operations. This does not introduce a parser keyword,
FIR opcode, format version or implicit conversion. Dynamic string/key-to-big
conversions remain outside this proof and retain their allocation/error effects.

| Source and destination | Bit facts |
| --- | --- |
| int32 → big | Copy low32; high32 follow the proven sign bit |
| big → int32 | Keep only low32; discard all high-bit knowledge |
| bool → int32/big | High bits zero; low bit remains unknown for a dynamic bool |
| same integer type | Preserve existing bits/copy semantics |

An unknown sign leaves widened high bits unknown. A nonzero big source does not
prove a narrowed int32 is nonzero:4294967296 narrows to0. Fully determined typed
conversions may become exact literals, while operand producers remain evaluated.
Checked table reads and division-by-zero remain observable. The existing effects
and memory barriers are unchanged; no cross-heap pointer or ownership rule changes.

Examples: widening `x & INT32_MAX` then arithmetic shifting by63 yields0;
widening `x | INT32_MIN` then shifting yields-1; narrowing a big with all low32
bits forced one yields-1. A dynamic boolean converted to either width and masked
with-2 yields0. This is bit truncation/sign extension, not a floating-point rule.

## Compile the source example

From the installed FIR directory (adjust executable paths for your platform):

```sh
./fir1_source limbo --input examples/dis/cast-facts.b raw.fir
./fir1 opt raw.fir opt.fir
./fir1 dis raw.fir raw.dis
./fir1 dis opt.fir opt.dis
../disvm-editor/disvm --no-jit --inspect-words 0 2 raw.dis
../disvm-editor/disvm --jit --inspect-words 0 2 opt.dis
```

Both return DATA0=42 and the success flag DATA8=1. Every source line is commented;
explicit int aliases keep int32 literals separate from the big-capable table
profile. The C99 source-library SDK client emits identical Dis bytes to the CLI.
These mixed-width tests target Dis; they do not imply general big SWF support.

The independent C99 basis oracle checks137 graphs /35,620 output values against
all single-bit and complementary inputs, zero/all-ones and sign/high boundaries.
It requires useful exact masks, immutable IR and a later textual dominating block.
The target matrix checks134 raw/optimized values plus eager-error and narrowed-zero
divisor cases, optimizer fixed points and the own Coco/R source example. Both
interpreter and JIT execute the actual emitted Dis. Full-width literals remain
handled by the existing signed-payload helper without host-int truncation.
