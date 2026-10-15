# Borrowed Array end operations with negative length — v0.374

Loaded SWF4–8 borrowed push/pop/shift/unshift now accept own lengths wrapping to
negative Int32. Push writes signed own keys starting at captured length, including
crossing zero, then stores the signed sum as length and returns that sum. Unshift
has no source range to move, deletes/writes indices starting at zero, returns the
signed sum and leaves borrowed length unchanged. Pop reads/deletes the raw own
key at length minus one without resizing. Shift returns raw own index zero;
there is no movement, deletion or borrowed length update for negative length.

Own data reads ignore prototypes and do not invoke getters. Existing raw-store
and delete rules retain readonly/nondeletable flags and bypass setters/watches.
Insertion work is charged. Native invocation protects values awaiting return;
allocation failures restore roots/depth but may retain partial mutations. Native
Array negative storage, signed overflow (including pop at Int32 minimum) and
complete Flash compatibility remain open. Host mode is unchanged.

Comparison source: [pinned Ruffle Array implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
`array_negative_ends` covers all five loaded versions, signed keys/zero crossing,
zero arguments, preserved borrowed lengths, readonly/nondeletable data, raw
accessors, Int32 minimum, work limits and native rejection.
`array_negative_ends_allocation` injects each allocation boundary for all four
methods, collects after failure/success and retries partial writes.

Validation v0.374: all 771 AGG / 771 Cairo / 752 minimal Linux Release checks
have passing evidence after ELF execution-permission repairs and failed-test
reruns. Five restricted heap/global ASan+UBSan checks pass (stack ASan and LSan
disabled): both new end-operation tests, legacy push/unshift and negative splice.
