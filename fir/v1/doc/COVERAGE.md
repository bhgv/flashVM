# Current coverage — FIR tools 0.52

[Русский](COVERAGE.ru.md) · [Source grammar](../compiler/src1.atg) · [Public API](../include/fir.h) · [Compiler architecture](../../../compiler/README.md)

This page describes executable FIR adapters, separately from the broader standalone syntax frontends and native bootstrap. A recognized syntax tree alone does not establish lowering or execution.

| Component/profile | Executable coverage | Boundary |
| --- | --- | --- |
| FIR reader/verifier | Coco/R FIR 1.0, UTF-8, SSA, dominance, types, control-flow targets and terminators | No binary FIR or interprocedural analysis |
| AS1/2/3 source adapter | Numeric trace, initialized bindings, AS3 const, numeric annotations, arithmetic | Teaching AVM1 compatibility; no complete AS/ABC source compiler |
| Limbo homogeneous int | Signed32 arithmetic/division/remainder, bit operations, checked shifts, aliases, blocks and terminal comparisons | No implicit bool/int mixing or general source module ABI |
| Limbo homogeneous real | Binary64 arithmetic, initialized typed locals, aliases and lexical blocks | No mixed numeric conversion or general real CFG |
| Limbo typed big/table | Exact big/int arithmetic, bool comparisons/logical values, Unicode strings, hybrid tables, key snapshots, constructors, mutation/slices and typed loops | Restricted value() teaching entry; no arbitrary composite/type/function compiler |
| Type aliases | Forward file/module prefix, sequential body/block/branch/loop aliases, canonical casts and scope retirement | 128 declarations per request; no imports/qualified/generic types or local forwards |
| Optimizer | Verified explicit optimization, constant folds, CFG simplification, conservative memory/exception effects | Not a whole-program optimizer; effectful producer errors remain observable |
| Dis targets | Direct scalar and hosted NC64 emission with original/optimized execution checks | General Dis linking, source channel lowering and a complete compiler/runtime ABI migration remain separate |
| SWF target | SWO1 objects, separate SWL/SWA1/resources to SWF8; numeric bits and raw FIR AS properties | No ABC/AVM2 or live checked-shift exceptions; extensions require their explicit target contract |
| C99 libraries/debug origins | CLI/library paths, staged source rejection and original UTF-8 byte spans | Generated parser remains serialized; no complete IDE debugger attachment |

## Contracts and tutorials

- [Exact source syntax](SOURCE_SYNTAX.md), [lexical value scopes](LEXICAL_SCOPES.md), [alias scopes](SOURCE_ALIASES.md) and [diagnostics](SOURCE_DIAGNOSTICS.md).
- [Signed32 bit primitives and shifts](INT_BITS.md), [int arithmetic](INT_ARITHMETIC.md), [typed int shifts](INT_SHIFTS.md) and [text/key conversion](TEXT_VALUES.md).
- [Hybrid constructors](TABLE_LITERALS.md), [table ownership](TABLES_DIS.md), [Unicode strings](UNICODE_STRINGS.md) and [slice replacement](SLICE_ASSIGNMENT.md).
- [Dynamic any status](DYNAMIC_VALUES.md), [Dis backend](DIS_BACKEND.md) and [commented examples](../examples/README.md).

The current source limits are 64 KiB input, 255-byte tokens, 256 admitted value names, 128 type declarations, 1000 SSA values and shared depth 64. Limits include retired declarations where documented. Rejected source preserves existing FIR/debug output; a later host I/O copy failure is not a rollback transaction.

The teaching wrapper calls/traces value(); it is not Inferno's general init ABI. The native Dis runtime/IDE also uses an explicit source-built bootstrap for ordinary service modules until our own compiler covers those languages. Runtime channel/projection libraries and raw FIR channel operations do not imply source channel/alt/spawn lowering in this adapter. ARM64 emission/emulation is separate from physical-device execution.
