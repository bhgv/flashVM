# Dis integration acceptance sequence

Planned work, not an implementation or a passed test report. [Русский](DIS_ACCEPTANCE.ru.md).

1. **Isolate runtime.** Build a reusable Dis interpreter library with explicit source revision and licenses. Link/registration inspection must prove absence of Inferno Draw/Tk/Prefab and crypto/Keyring. Boot a real compiled Limbo module; no successful dummy module replacements.
2. **Host lifetime.** Define loader, allocator, scheduler budgets and shutdown adapters. Exercise strings, arrays, exceptions, module calls, spawn, buffered/unbuffered channels and alt. Validate heap roots and teardown independently of the FlashVM collector. Keep interpreter/JIT acceptance separate.
3. **Local channel bridge.** Run real Limbo send/receive against a typed FlashVM resource endpoint. Requests are copied and queued to the VM owner; replies and events return through channels. Cover blocked sends/receives, cancellation, queue limits, disconnect, stale generations and pending-operation teardown without blocking display/input pumping.
4. **First UI control.** A Limbo module creates a MovieClip/TextField through the channel service, changes text, receives a real input event and destroys the control. Add missing channel operations; do not bypass the boundary with a direct application call. Unknown capabilities return unsupported.
5. **Editor slice.** Implement a Limbo editor document and complex control composed from FlashVM primitives: text editing, cursor/selection, focus, scrolling, keyboard input and a dirty-document indicator. Open/save and command replies use channels. Rendering a label is not this acceptance gate.
6. **Build/debug services.** Invoke native compiler/optimizer/linker interfaces behind channels and return correlated diagnostics. Connect launch/stop/debug operations. The deployed IDE must not require the host Python driver. Limbo compilation uses the real compiler, not the numeric FIR teaching subset.
7. **Distribution and ports.** Demonstrate the same Limbo-facing operations over a second transport without changing UI logic. Test ordering, backpressure and failures explicitly; do not claim network distribution from local channels alone. Package IDE modules with FlashVM and validate selected OS/CPU/device combinations separately.

Each step needs executable tests and a concise EN/RU contract before it is marked implemented. There is no promise that all existing Dis JITs or every Inferno library can be embedded unchanged. Missing FlashVM primitives may be explicit built-in extensions; SWF <= 8 compatibility and extension acceptance remain separate.

## Mandatory JIT/debug target gate

After interpreter/bridge bring-up, accept x86/i386, x86-64, arm64 and riscv64 separately with actual JIT-entry evidence and debugger parity. Interpreter-only success does not close this gate. Isolate the runtime under flashVM/external/dis/. See [required debugger contracts](ARCHITECTURE.md).
