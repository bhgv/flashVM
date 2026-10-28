# Numeric counted loops — FIR 0.58 / source CLI 0.25

[Русский](NUMERIC_FOR.ru.md) · [Numeric flow](NUMERIC_FLOW.md) · [Source syntax](SOURCE_SYNTAX.md) · [Int tutorial](../examples/dis/numeric-for-int.b) · [Real tutorial](../examples/dis/numeric-for-real.b) · [Nested scopes](../examples/dis/numeric-for-nested.b)

Homogeneous Limbo int/real entries accept counted for loops and statement-only
postfix updates. They use the same Coco/R source grammar and public source
library as the CLI. The numeric profile remains a bounded value() entry;
full function/module ABI and numeric snapshot for-in are outside this contract.

## Header and execution order

```limbo
sum := 0; # Keep an outer accumulator across all loop edges.
for (i := 0; i < 10; i++) { # Initialize once, test before body, step after it.
    if (i == 3) { continue; } # Skip three, but execute the header step.
    if (i == 8) { break; } # Exit before summing eight or running the step.
    sum += i; # Sum precisely the seven contributing indices.
} # Retire header i while preserving outer sum.
return sum + 17; # Return twenty-five plus seventeen: forty-two.
```

The initializer executes once. The bool condition executes before each body,
including the first. Normal body fallthrough executes the step once, then tests
again. Continue targets this step; break and function return skip it. Nested
while/for transfers target the innermost loop. The sample sums 0, 1, 2, 4, 5, 6,
7 and returns 42. Real follows the same edges with binary64 operands.

Each header part is optional, but both semicolons and braces are required.
An omitted condition means true. As with while, the conservative return proof
still permits an empty loop: a return after the loop is required on its open
continuation. Unreachable suffixes after break/continue/return reject.

| Position | Accepted clause | Scope/effect |
| --- | --- | --- |
| Initializer | Initialized inferred/explicit numeric declaration, assignment, compound assignment or postfix update | Runs once before test |
| Condition | Existing homogeneous numeric/bool expression yielding normalized bool | Re-evaluates on backedges |
| Step | Assignment, compound assignment or postfix update | Runs only on fallthrough/continue |
| Body | Existing numeric structured statements, nested loops and lexical blocks | New lexical scope per source block |

One clause is accepted per initializer/step; comma lists, header type aliases,
uninitialized declarations, prefix updates and update expressions reject.
Postfix x++/x-- also work as ordinary statements. They produce no expression
value and update by exact one of the selected kind. Int wraps modulo 2^32;
real uses binary64 addition/subtraction, including signed-zero/NaN behavior.
No implicit integer-to-real source conversion is added.

## Scope and staged lowering

The initializer has a loop-local scope visible in its condition, step and body.
The numeric body has a nested scope: a body-local i can shadow header i without
changing which binding the staged step updates. After the loop neither header
nor body declarations escape. Assignments to outer locals persist. Alias
resolution and bindings use their existing separate namespaces.

The source compiler promotes active numeric locals to the private i32.slot or
f64.slot storage documented in NUMERIC_FLOW. It parses the step once into an
owned temporary stream, then restores incoming binding facts before parsing the
body. It emits that stream at the step block only when fallthrough or continue
can reach it. A wholly returning/breaking body contributes no dead step or
step debug sites. Nested loops own separate streams; failure and the next API
call retire pending streams/state. Existing SSA, block and depth budgets apply. Lexical scope admission is bounded
independently: a counted header and its body need two scopes even though the
parser descends once. After a sticky error, scope entry/exit freezes until reset,
so error recovery cannot index an unadmitted owner.

No FIR enum, runtime frame layout, Dis opcode or SWF action is added. Verifier,
effects and optimizer retain the established numeric-cell contract. All control
edges reach the existing Dis interpreter/JIT and AVM1 SWO/linker. Backend limits
remain explicit: checked int division/remainder/shifts can require Dis, even
when authored in a counted header; SWF rejection remains transactional.

FIRDBG1 retains UTF-8 byte spans: legacy while-test events identify for tests,
for-end identifies the loop exit, and binary events for ++/-- use the two-byte
operator. Deferred step events are published after body events, in emitted CFG
order. This is source/FIR provenance, not a final Dis/SWF/native-PC debugger map.

## Build, run and embed

```sh
build/fir/fir1_source limbo --input fir/v1/examples/dis/numeric-for-real.b /tmp/for.fir -g /tmp/for.debug.json
build/fir/fir1 check /tmp/for.fir
build/fir/fir1 opt /tmp/for.fir /tmp/for.opt.fir
build/fir/fir1 dis /tmp/for.opt.fir /tmp/for.dis
build/runtime/disvm --no-jit --stats --inspect-words 0 2 /tmp/for.dis
build/runtime/disvm --jit --stats --inspect-words 0 2 /tmp/for.dis
build/fir/fir1 object /tmp/for.opt.fir /tmp/for.swo
build/linker/swl link /tmp/for.swf /tmp/for.swo
```

The real tutorial captures binary64 42.5, signed-word bits 4631178160564600832,
and an executed flag of one. The int and nested tutorials return 42; the nested
tutorial has multiple possible trace sites, so inspect twice its emitted trace
count and require exactly one executed flag. Every nonblank tutorial line is
commented. Relocated SDK tests relink a C99 source/Dis client, compare its bytes
with CLI output, and run raw/optimized real counted loops in both dispatch modes.

Embedding uses unchanged fir_source_debug, fir_parse/fir_verify, optional
fir_optimise, then fir_dis or fir_object. Rebuild matching libraries/headers.
numeric_for_api.c interleaves 16 successful compilations and 32 located semantic
rejections, checking caller FIR/debug stream positions. numeric_flow.py compares
independent small-bound sums, nested scopes, wrapping updates, signed zero,
continue, return, skipped failing steps and raw/optimized runtime captures.
