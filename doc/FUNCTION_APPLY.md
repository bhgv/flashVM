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

## v0.419: ordinary argument properties

Loaded capture reads decimal-named properties through the ordinary object bridge
on every source kind. Native Number/Boolean/String/Date lists therefore expose
own and inherited fields/getters instead of bypassing them through the raw table
API. Each getter receives the original source as receiver. Missing fields become
undefined. Earlier captured values remain snapshots if a later getter mutates
or removes their source slots. The host table-index contract stays unchanged.

`function_apply_properties` checks SWF4–8 across four native classes, inherited
script getters, traced NUL strings, collection after source mutation, missing
slots, plain and counted script failures, no target call after failure and retry.
Its first target assertion failed on v0.418. This uses the same pinned Function
reference above; complete __resolve/metatable, borrowed apply and large argument
semantics still require separate evidence. All nine stages remain open.

Validation v0.419 / Проверка: minimal Release 835/835; AGG Release 4/4 and
heap/global ASan+UBSan 4/4 apply/function tests. Native mode-restoring launchers
used; stack ASan/LSan disabled. Other-platform runtime acceptance remains open.

## v0.420: incremental loaded capture and shared work

Loaded apply no longer rejects positive Int32 lengths above 65536. It grows the
rooted argument vector one visited slot at a time, charging before lookup and
allocation. Boxing, raw length conversion, legacy name fallback, getters and the
target share the caller's AVM1 work scope; nested callbacks cannot reset it.
Each legacy length-name candidate and each argument visit consume one unit.
The host retains its 65536 limit and unbudgeted capture. Negative signed lengths
remain UNSUPPORTED. This supersedes the loaded upper bound in v0.418.

`function_apply_work` checks exact quotas, nested parseInt conversion, inherited
and saturated counters, bounded legacy name search and retry. A sparse 65537-item
list reaches both a C callback and a prepared script body, whose arguments Array
and last slot survive real GC. An Int32-max list with quota four stops before
target invocation. `function_apply_allocation` sweeps failures through success
and checks scope/invocation cleanup; a 4096-byte allocation ceiling proves the
huge-list/quota-four path avoids full-length preallocation. Host quota isolation
is checked. `jit_function_apply` compares interpreted/attached/automatic native
CallMethod work counts, quota failure/retry, fractional length and primitive lists;
actual native entries are asserted. The quota regression failed on v0.419.

These are local-runtime checks, not large RPC or physical-platform certification.
General value validation, lookup internals and target activation work are not
fully charged. Unlimited-quota calls still depend on available memory. Complete
negative-length/borrowed-function compatibility and all stages remain open.

Validation v0.420: Linux minimal 838/838; AGG 7/7; heap/global ASan+UBSan 7/7.
Generated native-mode launchers compensate for workspace executable-mode resets.
Stack ASan and LeakSanitizer are disabled in this environment. Other platforms
and completion of all nine stages remain unverified/open.
