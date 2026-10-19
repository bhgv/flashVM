# Coverage and boundaries

[Русский](COVERAGE.ru.md).

| Component | Implemented | Not implemented |
| --- | --- | --- |
| FIR reader | Coco/R grammar, UTF-8, comments, exact version | Future versions, binary encoding |
| Verifier | SSA uniqueness, dominance, types, targets, reachability, terminators | Interprocedural analysis |
| AS1/AS2/AS3 teaching adapters | One or more `trace(expression);`; literals, parentheses, +, -, * | Strings, classes, properties, full source semantics |
| Limbo teaching adapter | Exact `implement Name; Name: module { value: fn(): int; }; value(): int { return expression; }` | General modules, channels/source operators, procedures, imports |
| Expressions | Decimal literals, unary minus on literals; * before +/-, left associative | Division, unary minus on groups |
| FIR channels | Create/send/receive/close mapped to existing extension | Source Limbo channel lowering, alt, spawn, capacity, typed graphs |
| SWF backend | FWS8, one trace-only frame, DoAction AVM1 | ABC/AVM2, visual assets, linker/imports, exceptions |

AS numeric adapter results use f64. Limbo literals must be integer spellings fitting i32 and arithmetic
wraps. The Limbo tutorial harness prints value() rather than implementing an
Inferno loader ABI. Source limit is 64 KiB, 1000 generated values, 64 nested groups.
Unknown syntax fails; no unsupported statement is silently discarded.

The AS3 tutorial is a restricted arithmetic compatibility profile emitted as
AVM1, not general AS3 support. Full AS3 needs a distinct AVM2/ABC backend.
The existing [compiler frontends](../../../compiler/README.md) remain independent,
with their own broader syntax acceptance; these adapters do not replace them.
The separate [linker](../../../linker/README.md) is not bypassed for future
multi-object builds: this initial emitter handles one self-contained module.
Future FIR types/operations must preserve language semantics before LLVM export.

## 0.2.0 update

Numeric adapters now accept initialized declarations (`var x = 1;` in AS, `x: int = 1;` inside Limbo), assignments and variable reads. AS2/AS3 allow only `:Number` annotations; AS1 rejects annotations. One program-wide environment holds at most 256 names. Duplicate declarations, reads before initialization and implicit globals fail. This restricted profile does not implement AS hoisting or nested scopes. AS accepts decimal exponents; Limbo requires integer spellings. Semantics live in `compiler/lower.c`, syntax in `compiler/src1.atg`.
