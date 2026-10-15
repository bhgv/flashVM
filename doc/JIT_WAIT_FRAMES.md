# Native frame waits and counted skips — v0.367

[Русское описание](JIT_WAIT_FRAMES.ru.md)

WaitForFrame and WaitForFrame2 lower to shared frame-availability queries plus
native conditional branches. Direct requests are immutable unsigned frame values.
Dynamic requests stay on the rooted operand vector through conversion and are
popped only after success. A loaded missing input uses a local undefined value
without changing the stack; host missing input is an error. The existing shared
service retains its distinct direct/value thresholds, strict decimal conversion,
wrapping/slop and intrinsic selected-timeline query policy.

The compiler records skips in complete Action units, separately from ordinary
byte-displacement branches. It resolves normal branches first, then maps each
wait's skip to an Action index. A skip reaching or passing the remaining records
uses a shared synthetic early exit. Its success helper clears the native status
register after a true predicate without charging another checkpoint. Ordinary
implicit End still has its interpreter-equivalent checkpoint; only the early
exit bypasses it. Ordinary jumps cannot use a newly appended synthetic exit to
make an otherwise invalid source-byte target legal.

| Test | Coverage |
| --- | --- |
| `jit_wait_frames` | Host/SWF4–8, interpreter/manual/automatic modes; ready/unready direct and dynamic checks, skip counts 0..3/255, variable-size records, chained waits, strict text conversion, missing operands, lower stack values, early undefined result; one-instruction budgets for explicit End and interpreter/automatic loaded implicit End |
| `jit_wait_callbacks` | Exactly one collecting/throwing toString, no valueOf, receiver after global detachment and GC, ready/unready results, intrinsic frame reads bypassing script accessors |
| `jit_wait_allocation` | Host/loaded direct/dynamic/early-exit compile and invocation allocation failures, preserved existing handles, restored roots/depths/active code and collection after failed calls |
| `jit_invalid` / `execution_modes` | Malformed wait widths and invalid source-byte jump to synthetic exit; automatic fallback now uses an unused anonymous DefineFunction instead of the newly supported WaitForFrame |

Unreachable unsupported Actions still conservatively reject compilation; this
change does not add native With/function/Try blocks. These are shared FlashVM
semantics, not full streaming or reference Flash compatibility, and not real
foreign-OS VM acceptance. All roadmap stages remain open.

Linux Release: all 750 AGG, 750 Cairo and 731 minimal checks pass after generated
ELF permission repairs before execution. Five wait/callback/allocation/invalid/
mode checks pass restricted heap/global ASan+UBSan; stack ASan/LeakSanitizer are
disabled. Freshly generated fixtures pass the existing 26 Unicorn synthetic
ISA/ABI regression cases; they do not execute these Actions in a foreign VM.
