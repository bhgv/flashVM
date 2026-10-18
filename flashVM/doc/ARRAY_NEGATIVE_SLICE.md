# Loaded slice with negative own length — v0.375

Loaded SWF4–8 slice retains the signed own length instead of replacing negative
length by zero before converting the bounds. For a negative length without
signed overflow, a nonnegative bound selects that length and a negative bound
selects zero. Consequently slice(0, -1) on length -3 copies raw own keys -3,
-2 and -1 into a dense three-element native result. Omitted/undefined end keeps
the captured signed length and produces an empty result in that example.

Length is converted before start and end. Later bound callbacks can change
source length/data but do not change the captured range. Raw own reads ignore
prototypes and bypass getters; holes/accessor data use the same rules as loaded
positive slice. Source data is not modified. Both borrowed objects and native
Arrays whose stored unsigned length wraps negative are supported, without
changing native length representation. Results are rooted while filled and each
copy is charged to the shared work budget.

Signed bound-addition overflow and result lengths above Int32 maximum remain
explicitly unsupported. Native negative-length mutation remains incomplete;
no roadmap stage is closed. Host slice is unchanged.
Comparison: [pinned Ruffle Array implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/array.rs).
Tests `array_negative_slice` and `array_negative_slice_allocation` cover SWF4–8,
native/borrowed sources, dense undefined holes, raw accessors, omitted/undefined
bounds, length-changing conversion, integer bounds, budget failure, allocation
sweeps, unchanged sources and retained strings after GC/retry.

Validation v0.375: 773 AGG / 773 Cairo / 754 minimal Linux Release checks
have passing evidence (AGG/Cairo ELF permission repairs and failed-test reruns;
minimal passed the full run directly). Five restricted heap/global ASan+UBSan
checks pass: both negative-slice tests, both negative-end tests and legacy slice.
Stack ASan and LeakSanitizer remain disabled in this environment.
