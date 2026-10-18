# Native legacy comparisons and logic — v0.359

[Русское описание](JIT_LEGACY.ru.md)

Equals, Less, And and Or now lower to direct native helpers calling the shared
legacy-comparison and logical services. Equals performs numeric comparison,
converting right before left; Less converts left before right. Existing `__eq`
and `__lt` hooks retain left-side priority and right-side fallback, rooted hook
arguments and the service's result-truth projection.

And/Or convert both inputs left-to-right through the shared Boolean policy;
conversion is not short-circuited. Loaded SWF4–6 strings use numeric truth, while
host/SWF7–8 strings use nonempty-string truth. For example, `"0" And true` is false
in SWF4–6 and true in host/SWF7–8. These legacy operations remain distinct from
Equals2/Less2/StrictEquals and string-specific comparison Actions.

Both original inputs remain on the GC-rooted invocation stack through conversions
and hook calls. A successful result replaces the first operand before popping the
second, with no allocation after the shared service releases its temporary roots.
Errors propagate through the existing native safe boundary.

| Test | Coverage |
| --- | --- |
| `jit_legacy` | 1944 primitive pairs/operations across host/SWF4–8, each interpreted and explicitly/automatically native; missing operands; independent numeric equality/ordering and versioned `"0"` truth examples |
| `jit_legacy_callbacks` | Collecting/throwing numeric coercion with exact order; first-class collecting/throwing primitive metamethods; left priority and right fallback; positional hook arguments and non-Boolean hook result truth |
| `jit_legacy_allocation` | Host/loaded allocation failures through all four Actions, collecting conversions and left/right hooks; restored roots, depths and active handles |

The shared differential fixture now initializes its output before a call that may
fail, so a failed baseline never copies uninitialized result fields. All roadmap
stages, remaining Action support and foreign-target runtime acceptance remain open.

Linux Release: all 729 AGG, 729 Cairo and 710 minimal checks pass after restoring
generated ELF execute permissions. Expanded hook cases were explicitly rebuilt
and pass in every profile and restricted heap/global ASan+UBSan. Six focused
legacy/string sanitizer checks pass; stack ASan and LeakSanitizer remain disabled.
All 26 current synthetic backend ABI/control-flow cases also pass in Unicorn
2.1.4 (JIT_ARCHITECTURES.md); these use stub helpers, not a full foreign VM.
