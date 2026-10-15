# Native trace, throw, time and random — v0.363

[Русское описание](JIT_EFFECTS.ru.md)

Trace, Throw, GetTime and RandomNumber lower to native helpers that use the same
execution and exception services as interpretation. Trace retains its input until
conversion and synchronous sink delivery succeed. Conversion still runs with no
sink; sink failures propagate without replay by default. Throw publishes its
value in the exception roots before consuming the operand and unwinds native code.
GetTime reserves bounded operand storage before reading the context clock.
RandomNumber keeps its bound rooted through numeric conversion and draws from the
same context state as Math.random only after a positive integer bound is prepared.

`jit_effects` uses isolated host/SWF4–8 contexts and interpreter/manual-native/
automatic-native execution. It checks exact random sequence and skipped draws,
collecting/throwing bound conversions, integer monotonic clock samples, counted
NUL trace text, disabled/working/failing trace sinks, collecting/throwing toString,
Throw exit and missing operands. Collecting callbacks remove their global binding
before collection and then read their receiver, exercising the live operand roots.
`jit_effects_allocation` injects allocation failure through each new operation in
host and loaded contexts, verifies root/depth/active-code restoration, and verifies
that a thrown string survives code destruction and collection.

The automatic fallback fixture now uses WaitForFrame, which remains unsupported
by JIT, instead of GetTime; the invalid-input test uses unsupported Play instead of Trace.
These helpers do not implement Try or exception
handlers, a reference Flash clock/random oracle, all Actions, or foreign-OS VM
execution. No roadmap stage is closed.

Linux Release: passing evidence for all 738 AGG, 738 Cairo and 719 minimal checks,
including reruns after generated ELF permission repairs. Four effects/allocation/
mode/invalid checks pass restricted heap/global ASan+UBSan; stack ASan and
LeakSanitizer are disabled.

The end-to-end platform validator with --profile all --jit-emulator also passes
all three profiles and 26 Unicorn 2.1.4 synthetic ISA/ABI cases per profile after
repairing an initial Cairo ELF permission failure. This is not foreign-VM evidence.
