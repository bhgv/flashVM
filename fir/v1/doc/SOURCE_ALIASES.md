# Source aliases → canonical FIR (0.50)

[Русский](SOURCE_ALIASES.ru.md) · [Runnable tutorial](../examples/type-aliases/README.md) · [Interface resolver](../../../compiler/limbo/frontend/ALIASES.md)

The executable source adapter uses the authoritative [Coco/R grammar](../compiler/src1.atg) and [lowering library](../compiler/lower.c). Both CLI and serialized C API accept file aliases between implement and the module declaration, and implementing-module aliases before the value export. A declaration uses `Name: type int;`, `Name: type real;`, `Name: type big;`, `Name: type string;`, `Name: type table of big;` or another alias in the same complete declaration prefix.

The prefix stores exact named edges, then resolves all declarations iteratively before parsing the value export. Forward references bind canonically in the declaring lexical scope. An earlier file alias remains unchanged when its source name is shadowed inside the module. The implementing body sees module aliases. Numeric results accept int/real/big aliases. Numeric int/real locals remain homogeneous; the big/table profile accepts int, big, string and table-of-big annotations. String/table results and general composite type expressions are unsupported. Aliases emit no FIR instructions: tests require byte-identical FIR to direct canonical annotations.

The adapter bounds aliases to 128 per translation unit and names to the existing 255-byte token buffer. Duplicate/unknown/cyclic aliases (including unused declarations) and value locals shadowing aliases are semantic errors. Local type declarations, qualified/generic types and includes are not implemented. The separate interface resolver remains the broader interface/AST service; its catalog is not substituted for this bounded executable source prefix. This is not a complete typed-body compiler.

State resets for every library request. Syntax/semantic failures preserve existing staged FIR and debug outputs. Malformed `implement ;` and a missing module stop with diagnostics; the prefix repetition explicitly requires an identifier to guarantee parser progress at EOF. Debug origins continue to refer to the original source snapshot. Adding the type keyword preserves AS1/2/3 variables named type and table fields such as t.type.

Tests compile int/real/big/table aliases, compare canonical FIR, optimize and emit real Dis images, then execute both interpreter and JIT paths with quantum 1 and 2048 and inspect results/dispatch counters. Optional SWF acceptance compiles numeric FIR to SWO, links SWF and inspects FlashVM trace; hybrid tables remain Dis-only. Existing direct Dis emission is reused. This work introduces no new runtime opcode, cross-heap pointer or native module ABI. The tutorial's value() entry is a teaching wrapper, not Inferno's general init interface.

## Alias casts and diagnostics

An alias identifier in unary-expression position is selected by a read-only semantic predicate over the exact Coco/R lookahead token. No second lexer or source-text parser is used. The target and authored token mark are captured before recursive operand parsing. Prefix casts bind like primitive casts: `I text + big 1` converts only text, while `I (expression)` converts the whole group. The shared depth-64 limit remains active.

In the typed big/table profile, int/big/string alias casts reuse [checked conversions](TEXT_VALUES.md): signed64 text parsing precedes signed32 narrowing, snapshot keys retain their captured value, and invalid unused parses remain observable after optimization. Unsupported real/table cast targets are rejected before publication. In homogeneous int/real profiles only a matching identity alias cast is accepted; mixed numeric conversions remain unsupported. Identity casts emit no extra FIR instruction.

Every named edge retains its RHS byte span. Missing targets and cycles report that span after UTF-8 prefixes; successful resolution restores the original use token. The bounded 128-identity path and canonical cache use no heap allocation or depth-proportional host recursion. Catalog state is request-local and is reset after both successes and failures. Tests include a full forward chain, a 128-edge cycle, wrong-scope targets, unknown unused aliases, and repeated C API recovery.

## Inferno comparison

The optional [oracle](../tests/alias_oracle.py) compares common unshadowed int/real forward aliases and prefix casts against explicitly supplied source-built Inferno compilers. Both classic32 and NC64 produce byte-identical alias/direct Dis images when the input pathname is held constant. This verifies compilation, not execution of a foreign ABI.

Module/file alias shadowing in the same source is a FlashVM lexical extension: the selected Inferno compiler rejects that redeclaration. Hybrid tables are also outside the common oracle. Neither extension is exported upstream. Run from the repository root with built reference tools:

```sh
export TMPDIR="$PWD/build/tmp"
python3 fir/v1/tests/alias_oracle.py build/limbo-classic/limbo-bootstrap build/runtime/bootstrap/limbo-bootstrap
```
