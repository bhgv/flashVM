# Our Limbo compiler: priority and interface contract

[Русский](OWN_COMPILER.ru.md) · [Syntax/API](frontend/README.md) ·
[Dis/SWF external modules](examples/external-modules/README.md) ·
[Source debug](../../fir/v1/doc/SOURCE_DEBUG.md)

The primary implementation is our C99/Coco/R Limbo frontend and FIR pipeline.
The owner's C and Limbo compiler versions under tools/limbo-bootstrap/upstream
are reference/bootstrap sources, especially stubs.b and sbl.b. They are not
renamed and advertised as our completed compiler. Their upstream is
https://github.com/bhgv/Inferno-OS-bhgv, experiment, pinned in the tools manifest.

## Current executable steps

- The broader Coco/R grammar produces an owned concrete syntax tree and explicit
  `.m` declaration catalog with source spans. This is separate from semantic lowering.
- `limbo_bindings` consumes that catalog and emits C99/Python external scaffolds,
  external AS module adapters, AS proxies for Dis modules, or neutral JSON descriptors.
  The SAME `.m` header describes either a Dis or a non-Dis implementation and AS callers.
- The numeric Limbo→FIR adapter compiles lexical integer examples. `fir1_source -g`
  emits FIRDBG1 with the exact source snapshot, pre-optimization SSA origins,
  variable bindings and lexical scope events. It is not a final Dis/SWF PC map.
- The temporary local bootstrap compiles complex IDE Limbo to Dis/.sbl until
  our own frontend/lowering/backends cover that code. It needs no downloaded binary SDK.

## Required direction

1. Keep all new source parsers on Coco/R, with readable attributed grammars.
2. Add full semantic/type/module resolution and target-correct layouts to our Limbo,
   using both upstream versions as references. The Limbo version is important for
   eventual self-hosted portable tooling, not just the C bootstrap.
3. Preserve/relocate debug origins through FIR optimization, object emission/linking
   and Dis/SWF/JIT publication; expose functions, lexical variables and source maps.
4. Generate external modules from `.m` for C and AS as first-class targets, with other
   languages possible through the descriptor. Generate AS callers of Dis from the
   same header. Check real signatures and marshal values/capabilities, not private frames.
5. Execute Limbo/Dis and AS/SWF complementarily with separate ownership, scheduling
   and heaps connected by checked typed channels/modules. Possible later unification
   must be designed and verified explicitly; no merged container is claimed here.

Current AS adapters are generated source plus a transport contract, not an already
installed native module loader. Current C thunks are implementation starting points,
not guessed Dis ABI layouts. Includes, aliases, ADT layouts, structural signatures,
GC maps and complex-value marshaling still require the semantic/host layers.
Full AS source lowering/ABC and full own Limbo→Dis emission remain in progress.

[Structured function signatures / Структурированные сигнатуры](frontend/SIGNATURES.md)
