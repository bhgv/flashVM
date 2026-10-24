# Full Limbo compiler direction

Accepted requirements, not completed compiler features:

1. Implement a complete Limbo compiler in readable C99 and a Limbo implementation. Keep existing Dis module/type/channel/exception semantics and debug information compatible with the actively maintained experiment branch.
2. Use authoritative Coco/R .atg scanner/parser sources for both. Reuse bhgv/coco-r-limbo, not a new generator port. Token/production coverage and diagnostic/source-span cases should be equivalent across implementations; embedded actions, attributes and predicates are output-language code and need C/Limbo-specific implementations.
3. The current broader C frontend only recognizes syntax, and the FIR value() adapter is a numeric subset. Neither is a complete replacement for the real compiler. Next semantic stages need owned AST/source spans, scope/import/type/signature checks, typed lowering and a Dis backend with .sbl emission.
4. Preserve and adapt existing Dis debug facilities. Extend symbol decoding beyond this source-map prefix before claiming local/type/function display. Keep compiler source positions, logical Dis PCs and native JIT maps correlated.
5. A later AS implementation uses an AS port of Coco/R and .atg too. AS-to-Dis/module interoperability is separate runtime work; a generated parser alone is not a compiler or working module bridge.
6. Recheck upstream fixes at implementation/import checkpoints. Core Dis stays isolated under flashVM/external/dis/ when imported, with existing x86 retained and new x86-64/arm64/riscv64 JIT/debug acceptance tracked separately. No Tk/Draw/crypto dependency is introduced into the IDE.
