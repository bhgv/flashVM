# Limbo IDE hosted by FlashVM

Status: accepted product direction, integration not implemented. [Русский](ARCHITECTURE.ru.md).

## Product boundary

The IDE application, its editor/document/project model and complex widget logic are to be written in Limbo. It runs inside the FlashVM application through an embedded Dis runtime. FlashVM supplies all rendering, MovieClips, text, input and available platform facilities. There is no separate Inferno desktop and no dependence on its Draw graphics core, Tk, crypto/Keyring or Prefab UI stack.

This deliberately targets a superset of SWF <= 8. Required missing native capabilities may extend FlashVM built-in classes through explicit channel operations. They are advertised as extensions; adding an extension does not certify full legacy SWF conformance.

The current [Python driver](build.py) remains a host development/test utility. It is not the target IDE implementation and must not become a mandatory deployed runtime dependency. The existing native C99 compiler/optimizer/linker APIs can later be offered by build services through channels. No new compiler service or Limbo application is implemented by this design document.

## Execution choice

Use the real Limbo compiler and Dis bytecode as the initial full-language route, rather than depending on translation of arbitrary Limbo to AVM1. The current [FIR source profile](../fir/v1/doc/SOURCE_SYNTAX.md) is a numeric value() subset and cannot host the IDE's processes, typed channels and general module/library use.

Dis is an embedded execution engine inside the FlashVM host, not a separately launched emu executable. Initial acceptance uses the interpreter; target-specific Dis JIT work remains separate from AVM1 JIT generation. Existing MIPS, SPARC and PowerPC JITs are outside this integration. Future x86-64, AArch64/arm64 and RISC-V64 ports must preserve Dis value/layout semantics independently of native pointer width. A FlashVM platform port does not by itself prove a Dis port; the small remaining runtime adapter must be validated too.

## Dis extraction baseline

Read-only inspection pinned Inferno experiment at [218b4adf](https://github.com/bhgv/Inferno-OS-bhgv/tree/218b4adfef1e238e705dd13ef0f9636e79abad7e). Its [libinterp build list](https://github.com/bhgv/Inferno-OS-bhgv/blob/218b4adfef1e238e705dd13ef0f9636e79abad7e/libinterp/mkfile) includes draw/tk/crypt/keyring and related module generation; it is not a ready minimal embeddable library. [Interpreter types](https://github.com/bhgv/Inferno-OS-bhgv/blob/218b4adfef1e238e705dd13ef0f9636e79abad7e/include/interp.h) expose scheduler, channels, heap and module state. Current upstream fixes and their test coverage must be reviewed at each chosen import revision.

Extract loader, interpreter, scheduler, typed channels/alt, heap/GC, strings, exceptions and the required core modules behind a narrow host adapter. Identify and remove forbidden module registration/link dependencies and graphics-specific finalization hooks; do not merely replace graphics/crypto calls with successful no-op stubs. Reuse original sources and preserve their licenses/notices. No source import, executable Dis subset or boot acceptance is claimed here.

## Channel-only UI boundary

All IDE-originated control operations use typed request/reply channels, including local calls. There is no Limbo-to-MovieClip pointer or direct UI mutator API. Native channel handlers may invoke FlashVM C APIs on the VM execution owner; that is the implementation of the channel boundary, not an alternative application access path.

Reuse [resource dispatch](../flashVM/doc/RESOURCE_DISPATCH.md), [namespace design](../flashVM/doc/RESOURCE_NAMESPACE.md) and [event resources](../flashVM/doc/RESOURCE_EVENTS.md) where their contracts apply. The existing implementation explicitly does not supply general GUI methods or a complete Limbo transport. Missing channels must be added alongside each required control capability.

Proposed, not implemented, services:

| Service | Responsibility |
| --- | --- |
| Scene/widget commands | Create/remove controls and update supported MovieClip/TextField properties |
| Replies and input events | Correlated completion/errors; focus, keyboard, pointer and editing events |
| Documents/projects | Versioned text edits, file/resource operations and project state |
| Build/debug | Compile/verify/optimize/link/run requests, diagnostics and execution observation |

A complex control is composed from FlashVM primitives with Limbo state and channel traffic. A missing primitive should extend the corresponding built-in capability and its channel schema, rather than introducing another rendering toolkit. The first useful editor slice needs actual text editing, selection/focus, scrolling and keyboard events; a drawn text panel alone is not acceptance of an editor.

## Distribution and ownership

A local transport is first, but messages must not encode native struct layouts, pointers or heap references. Dis channel rendezvous and a remote serialized transport are not automatically interchangeable: the adapter must define copied payloads, request IDs, typed schemas, replies, event ordering, stale handle generations and bounded flow control. Use the same operation semantics across local and later IPC/network transports; do not introduce an in-process UI shortcut.

The FlashVM owner executes UI operations at safe points. A blocked Limbo receive/send must not block the rendering/event owner. Dis scheduling and FlashVM pumping need explicit budgets, cancellation and shutdown order. Accepted/queued is distinct from executed; replies report completion under each operation's contract. Local channels alone do not supply network discovery, remote mounting or exactly-once effects.

Dis and FlashVM retain separate collectors/heaps. Cross-runtime references use owner-managed handles and copied messages; native pointers are never serialized. Each side retains/releases its own roots, subscriptions and pending messages through completion/cancellation. Shared-heap cycles are not assumed to be collected automatically.

## Separate runtime directory, JIT and debugger

The intended import root is `flashVM/external/dis/`: Dis sources, required runtime support, licenses, build description, JIT backends and runtime tests stay isolated there. Only narrow host/channel integration belongs at the FlashVM boundary. Do not mix Dis heap, scheduler or instruction implementations into AVM1 core files. This directory is a planned destination; this patch does not import a runtime.

JIT support is a required product outcome, not an optional substitute for interpreter-only acceptance. Treat the user's x66 spelling as x86 for now; keep previously requested x86-64 explicitly. Required targets are x86/i386, x86-64, AArch64/arm64 and RISC-V64. Review/reuse applicable experiment backends, but track supported operations, interpreter fallbacks, instruction-cache synchronization, calling conventions, executable memory and target execution separately. A successful fallback is not proof that native JIT code ran. Existing excluded MIPS/SPARC/PowerPC backends are not modified.

The debugger is required for both interpreted and JIT execution. Its channel services must support attach/detach, stop/resume, breakpoints, step into/over/out, stack frames, locals/arguments, module and source positions, exceptions and selected channel/process state. Define which Limbo processes a stop affects without freezing FlashVM display/input pumping. Compiler debug information and mappings between source, logical Dis PCs and native code must remain available through JIT transitions, fallback and module reload. Native addresses are not public transport handles. Breakpoints and pending debug requests need module generations and explicit retirement on reload.

For each target, acceptance must demonstrate actual generated-code entry and parity with the interpreter, plus stops/steps, stack inspection, exception paths, GC-safe points and shutdown while debug work is pending. These are future acceptance requirements, not passed tests in this documentation patch.

## Packaging

Ship the Limbo IDE and required Dis modules with the FlashVM host/resources as one application distribution. Keep SWF <= 8 loading behavior intact. Dis module loading is an explicit optional runtime capability; the bytecode need not masquerade as AVM1 or require a new SWF tag before a format is designed. Unsupported builds must report the missing capability honestly.

[Acceptance steps](DIS_ACCEPTANCE.md) define the implementation sequence. Platform graphics/input ports are reused from FlashVM; Dis host adapters, packaging and actual platforms still require validation.
