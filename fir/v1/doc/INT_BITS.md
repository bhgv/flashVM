# Exact signed32 bits — compiler 0.52

[Русский](INT_BITS.ru.md) · [Bit tutorial](../examples/dis/int-bits.b) · [Shift tutorial](../examples/dis/int-shifts.b) · [API](../include/fir.h) · [Grammar](../compiler/src1.atg)

The homogeneous Limbo value(): int profile supports &, |, ^, unary ~, checked << / >> and their compound assignments. All operands/results are exact signed32, including on NC64. Local and prefix aliases remain canonical int types. AS and homogeneous real source profiles reject these operators; no implicit float/dynamic conversion is introduced.

Precedence is | below ^ below & below shifts below sums/products. Binary levels associate left; unary complement evaluates its operand once. Numeric conditions remain a single explicit comparison with internal bool results. Put a bit expression in a group, for example `if ((flags & mask) != 0)`. Ungrouped bit/comparison mixtures are rejected: native Limbo comparisons bind more tightly than &, and this bounded profile does not coerce its internal bool back to int.

## FIR and target lowering

Three append-only IDs spell i32.and, i32.or and i32.xor. Each has signature i32/i32 → i32 and zero effects. Existing IDs and FIR 1.0 remain unchanged. Complement lowers to xor with -1. The verifier rejects mixed operand/result types before either backend writes output. The optimizer folds using uint32_t bit patterns and a defined signed32 reconstruction, and may remove unused pure operations. Effects of operand producers remain conservative.

Typed big/table source int &, | and ^ also use the new primitive: one instruction replaces two widenings, one big operation and one narrowing. Checked text/key parsing that produced an operand is preserved. Full-width big operations retain their previous semantics.

Dis scalar and hosted emitters use existing ANDW/ORW/XORW plus signed32 normalization. No new Dis opcode, native module ABI or heap representation is introduced. SWF objects use standard AVM1 BitAnd/BitOr/BitXor; SWL 0.3.12 validates BitXor and retains its exact action bytes. Scalar live bitwise programs run through source → FIR → SWO → linker → SWF; no compiler-side SWF bypass exists.

Shifts reuse i32.shl.checked/i32.shr.checked. Counts must be int 0..31, right shift is arithmetic, left shift wraps signed32. Invalid counts raise even when their result is unused. A skipped branch does not evaluate its shift. Live checked shifts remain Dis/reference-only; the SWF backend preserves old output on rejection. Fully folded constants can use ordinary SWF constant coverage.

## Run and inspect

From the repository root after building matching tools:

```sh
export TMPDIR="$PWD/build/tmp"
mkdir -p "$TMPDIR"
build/fir/fir1_source limbo --input fir/v1/examples/dis/int-bits.b build/bits.fir -g build/bits.json
build/fir/fir1 opt build/bits.fir build/bits.opt.fir
build/fir/fir1 dis build/bits.fir build/bits.dis
build/runtime/disvm --jit --quantum 1 --stats --inspect-words 0 2 build/bits.dis
build/runtime/disvm --no-jit --quantum 1 --stats --inspect-words 0 2 build/bits.dis
build/fir/fir1 object build/bits.fir build/bits.swo
build/swf-linker/swl link build/bits.swf build/bits.swo
```

Both Dis modes report DATA 0 42 and DATA 8 1. The second word marks trace completion. Run the optimized image as well. The SWF result is 42 in a host exposing trace; an open window alone does not check it. For int-shifts.b repeat the Dis commands with a separate basename; it also returns 42. Every nonblank tutorial line is commented. Installed SDK tests compile both examples, relink a fresh C99 client and execute raw/optimized Dis in both modes.

Tests cover 192 endpoint/sign/operator combinations, precedence, aliases, compounds, branch PCs, both Dis emitters, loaded table operands, checked-count errors/lazy branches, source-kind rejection, UTF-8 operator spans and staged output. The C99 API test checks append-only IDs, pure effects, future-op conservatism and nine forged signatures. Native execution counters are checked; ARM64 hardware execution remains a separate acceptance step.
