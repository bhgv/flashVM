# Function.apply — roots and loaded semantics

## v0.417: preparation roots

Both unloaded host and SWF4–8 apply retain the boxed receiver while reading the
argument source, and root the initialized argument vector before any element
getter runs. Already captured strings, objects and cycles therefore survive GC
inside later script getters. The roots remain through target invocation and
unwind on success, errors, script exceptions and allocation failure. The borrowed
vector root helper allocates nothing and does not change invocation identity.

`function_apply_gc` supplies a fresh counted-NUL string and a fresh cyclic object
from two script getters; the third getter actually collects, checks live
registries without dereferencing reclaimed values and then supplies the final
argument. It checks null/boxed primitive receivers, plain failure, counted-NUL
exception, no target call after getter failure, cleanup and retry in host/SWF4–8.
The successful-path regression failed on the original v0.416 before this fix.
Existing standard_function checks remain in force.

This fixes temporary ownership, not complete apply compatibility. Loaded raw
length, versioned conversion, arbitrary argument counts, borrowed receiver
semantics, getter ordering and platform acceptance require separate evidence.
All nine roadmap stages remain open; the target is SWF ≤8 only.

Validation v0.417 / Проверка: minimal Release 833/833; AGG Release 2/2 and
heap/global ASan+UBSan 2/2 apply/function tests. Generated native launchers restore
workspace executable modes; stack ASan/LSan disabled. Foreign runtime acceptance
and all nine stages remain open.

## v0.418: loaded own length

In SWF4–8 a primitive argument-list value supplies no arguments. Object lists
capture own stored length without inherited lookup or getter dispatch, including
retained data behind addProperty and the native Array length slot. SWF4–6 fold
the length name; SWF7–8 use exact case. The captured value is converted once
through AVM1 Number/Int32 truncation and wrapping; undefined/NaN/infinity become
zero. A length object remains rooted through collecting conversion callbacks.
Receiver boxing precedes length conversion. The unloaded host contract is retained.

`function_apply_loaded` checks primitive lists, fractional/wrapped/invalid lengths,
raw virtual/inherited/native Array length, versioned names, a self-unbinding
collecting valueOf, errors/exceptions and retry. The primitive-list regression
failed on v0.417. Current loaded negative signed lengths and lengths above 65536
return UNSUPPORTED before allocation; this is a temporary runtime bound, not
full Adobe negative/large-length compatibility. Host still rejects invalid lengths
with INVALID_ARGUMENT. Mixed duplicate legacy key spellings remain uncertified.

Comparison baseline: pinned [Function implementation](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/globals/function.rs)
and [own data length](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
This is source comparison, not a fresh Adobe Player run. All stages remain open.

Validation v0.418 / Проверка: minimal Release 834/834; AGG Release 3/3 and
heap/global ASan+UBSan 3/3 apply/function tests. Native launchers restore workspace
executable modes; stack ASan/LSan disabled. Other-platform acceptance remains open.
