# IDE development rules

- The target IDE application and complex widget logic are written in Limbo and
  run in embedded Dis inside FlashVM. AS1/AS2 components and a later AS port
  are permitted through the shared module/channel interfaces. Python here is
  host development tooling.
- All application UI control uses typed channels, including local operations.
  Add missing channel capabilities instead of adding a direct UI access shortcut.
- Keep Dis and its required dependencies isolated under root disVM/;
  preserve upstream licenses and keep source provenance explicit. Do not import
  Inferno Draw/Tk/Prefab or crypto/Keyring. Do not mix Dis core with AVM1 core.
- Required runtime outcomes include x86-64, arm64 and riscv64 JITs and
  debugger parity with the interpreter. Keep the existing x86/i386 JIT as a
  reference/reuse baseline; no new i386 port is requested. Do not advertise an untested backend or
  interpreter fallback as native JIT execution. MIPS/SPARC/PowerPC remain outside
  the current integration scope.
- Write code for people: use consistent indentation, descriptive names and one
  logical statement per line. Expand branches and cleanup paths; split long
  conditions/expressions into readable groups. Avoid compressed one-line code.
- Put a useful comment before each function and type. Describe its purpose,
  ownership, blocking behavior and errors when relevant. Document each channel
  message/type/field, units, identifiers and ordering/response semantics.
- Explain non-obvious algorithmic decisions and invariants, not syntax already
  visible from the code. Keep Limbo source and native bridges equally readable.
- Start comments about GC roots, heap ownership transitions and finalization
  with [GC:]. Explain queue ownership, cancellation, stale handles, shutdown and
  JIT/debug safe points at the code that implements them.
- Apply readable layout/comments to handwritten tests too. Literal fixture data
  and generated/upstream code are not silently reformatted; preserve provenance
  and comment changed interfaces/adapters using the surrounding project style.
- Keep public native bridge APIs compatible with their owning component's C ABI;
  FlashVM retains its C89 public/C++98 implementation rules. Do not expose native
  struct layouts, pointers or heap addresses as a channel wire format.
- Maintain English/Russian contracts and explicit implemented/planned status.
  The architecture and acceptance documents are requirements, not runtime proof.
- Remain independently buildable/testable with explicit external tools. No hidden
  dependency on sibling source directories or mandatory deployed Python runtime.
- Prepare/test work branches and retain backup patches. The user's 2026-10-25
  instruction authorizes publication and bidirectional Inferno synchronization;
  see repository AGENTS.md. Author and committer must be bhgv.

- AS1/AS2 and Dis need bidirectional typed module adapters. Preserve separate
  heaps/owners, check signatures, retain pending-call roots and expose debugger
  boundary frames. Module invocation must not bypass channel-only UI control.

- Ship the IDE application as interpreted/JIT bytecode modules (Dis and/or SWF).
  Host Python and native compiler tools must not become the deployed IDE runtime.
  Keep SWF/Dis format unification and new container tags deferred until designed.

- Native legacy Limbo is a temporary source-built bootstrap in tools/limbo-bootstrap/,
  not the primary compiler implementation. Prioritize our Coco/R Limbo compiler,
  debug information and .m-driven external C/AS/other-language scaffolds.
- Dis and SWF are complementary execution engines with isolated owners/heaps and
  checked typed channel/module adapters. Possible later unification must not be
  assumed to exist or introduced by silently changing containers/ABIs.

- User instruction 2026-10-25: ship a free Unicode font (currently unmodified
  Noto Sans Mono/OFL), default editor size 11, with no font-location question.
  Ctrl+wheel and two-finger pinch must change scale through checked value/channel
  boundaries; touch input is not Android-specific. Preserve source/licensing and
  distinguish replay/dummy checks from physical desktop/touch acceptance.
