# Architecture decisions

1. One runtime shared by headless embedding and standalone player.
2. C89 public ABI; C++98 internal implementation; no STL across ABI.
3. Platform-neutral core; optional platform, graphics, media and JIT modules.
4. VM-managed objects exposed as opaque handles, with explicit GC rooting in a future API.
5. Feature flags must never silently enable unimplemented components.
6. Dynamic extension API will use a versioned function table and explicit lifecycle.
7. JIT targets: x86, x86-64, ARM32, AArch64, RV32 and RV64, with interpreter fallback.
8. SWF versions 1-8 and AVM1 are the compatibility target, not current functionality.
9. Error codes cross C ABI; C++ exceptions must never cross it.
10. Host-controlled execution limits, security and deterministic stepping are design goals.

## Planned modules

- swf: validated SWF tag parsing and decompression
- avm1: interpreter, scope, prototype semantics, GC
- jit: IR, code cache, architecture backends
- display: timeline, movieclips, text, shapes
- render: Cairo / AGG backends and offscreen rendering
- media: optional FFmpeg adapters
- platform: optional window/input integration
- extensions: C registration, native objects and callbacks

## SWF and AVM1 inspection (third milestone)

`fvm_swf_inspect` parses bounded FWS (uncompressed) files with SWF versions
1 through 8, checks declared length and tag boundaries, and reports metadata.
`fvm_load_memory` keeps a private copy of validated bytes in the context.
`fvm_swf_info` retrieves metadata for a loaded file. No timeline playback yet.
`fvm_avm1_inspect` checks instruction framing and termination for a standalone
AVM1 action block, with counts of Push and branch opcodes. It is not an
interpreter and does not validate every action's operands. CWS/ZWS, full tag
semantics, AVM1 execution and JIT remain unimplemented.
