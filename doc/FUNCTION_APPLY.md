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
