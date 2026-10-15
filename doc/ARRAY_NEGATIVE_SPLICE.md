# Borrowed negative-length splice — v0.373

Loaded SWF 4–8 borrowed `Array.splice` accepts an own length wrapping to a
negative Int32. A nonnegative start selects that signed length; removal is empty,
insertions use signed own keys and the final raw length is captured length plus
insertion count. Crossing zero is supported. Start, own length and delete count
are converted once in order. Later conversion mutations do not change bounds.
Negative starts without signed addition overflow, negative delete counts and an
explicit undefined delete count produce undefined. Missing delete count removes
zero entries. Unrelated tail properties remain intact. Raw stores use existing
property flags and bypass getters, setters and watches; insertion work is budgeted.
The removed array is temporarily rooted. Allocation/budget failure need not undo
previous insertions, consistently with existing generic splice.

Native Array negative storage and signed-addition overflow remain unsupported;
this does not close stage 1. Comparison source:
[Ruffle array implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Tests: `array_legacy_splice_negative`, `array_legacy_splice_length` cover signed
keys, zero crossing, captured bounds despite conversion side effects, omitted/
undefined/negative deletion, retained tails, readonly flags, budget exhaustion,
native negative rejection and root/depth restoration.
`array_legacy_splice_negative_allocation` injects every allocation boundary until
success, collects after failure, retries partial mutations and verifies retained strings.

Validation v0.373: 769 AGG / 769 Cairo / 750 minimal Release checks pass after
private-header rebuild and ELF permission repairs/reruns; five restricted
heap/global ASan+UBSan splice checks pass (stack ASan/LSan disabled).
