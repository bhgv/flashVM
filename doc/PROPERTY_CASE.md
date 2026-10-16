# Versioned property names

v0.423 adds a shared allocation-free stored-name selector using the existing
pinned Flash UTF-16 lowercase comparator. Loaded SWF4–6 own membership, attribute
queries and ordinary prototype-chain reads accept equivalent spelling. Host and
SWF7–8 keep exact names. Data, accessor and native method tables are covered;
queries never execute getters. Wrapped native methods use their stored spelling,
so reading a case variant preserves Function identity. Existing own assignments
select stored spelling; default intrinsic __proto__ aliases select its canonical
name. This avoids introducing a duplicate during a legacy update.

`object_property_case_queries` failed on v0.422 and covers SWF4–8/host, ASCII,
U+00C4/U+00E4, embedded NUL and lone-surrogate names, inherited reads vs own
membership, hidden flags, no getter/method execution, wrapped method identity,
legacy updates and real GC. Existing prototype/query tests remain regressions.
Reference: Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd,
[script_object.rs](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs)
uses activation.is_case_sensitive() for property membership and access.

This is a tested subset, not full name compatibility. Case-equivalent duplicate
slots supplied through raw/internal APIs prefer exact spelling and then map order;
Flash collision order is not certified. Flag mutation/deletion, inherited setter
spelling, watchers, definition APIs, extension metatables and full super membership
still need coordinated work. Linear folded lookup is not fully work-budgeted.
The original binary SWF corpus could not be restored through the text-only reader;
no new original-SWF pass count is claimed. All nine stages remain open.

Loaded SWF5+ no longer stores a synthetic ordinary this slot: activation-cell
lookup preserves the SWF5 local special-name rule (pinned function.rs load_this
sets registers only). Existing this/interpreter/JIT tests remain unchanged. The
coordinate test now expects folded public reads in SWF4–6; its coordinate
conversion assertions are retained. The class GC fixture uses ParentClass instead
of Super, which now correctly collides with the local implicit super name.

Validation v0.423: Linux minimal 841/841; AGG 8/8; heap/global ASan+UBSan
8/8. Generated native-mode launchers compensate for workspace mode resets.
Stack ASan/LeakSanitizer remain disabled. Other platforms and all stages open.

## v0.424 — mutate the selected own spelling

Flag updates and deletion select the own stored spelling before mutation. A
legacy alternate name now modifies/removes the same data, accessor or method
as its read and membership query. DONT_DELETE is checked before erasure; successful
deletion clears matching attributes, and inherited properties remain untouched.
Selection snapshots the name before erasing tables and remains inside the C API
allocation-failure boundary. Assignment shares the selector introduced here.
Host/SWF7–8 exact spelling and the pre-existing flags-only API policy remain.

`object_property_case_mutation` reproduced v0.423's mismatched flag update. It
checks SWF4–8/host, data/accessors/methods, NUL/Unicode names, DONT_DELETE failure,
clear/retry/delete, stale flags, inherited ownership, no callback execution and GC.
Inherited setter/watch/definition/collision/budget and full super semantics are
still open; this does not complete a roadmap stage.

Validation v0.424: all 842 Linux minimal cases passed (840 initial successes;
two process tests initially hit workspace child-executable PermissionError, then
passed 2/2 after restoring mode). AGG 6/6; heap/global ASan+UBSan 6/6. Generated
mode launchers used; stack ASan/LeakSanitizer disabled. Other OS/stages open.

## v0.425 — inherited setter selection and native execution

The ordinary setter chain now finds data/accessors with the same versioned name
selector as reads. Own data still shadows inherited setters; host/SWF7–8 exact
matching remains. Setter activation snapshots the accessor's stored name, not the
requested variant. Script setter re-resolution and root ownership therefore refer
to the actual descriptor even when callbacks delete it and detach the prototype.

`object_property_case_setter` first failed on v0.424. It checks counted names with
NUL across SWF4–8/host, receiver/owner/name identity, real GC, owner detachment and
descriptor deletion, plain errors, counted exceptions and invocation/root cleanup.
`jit_property_case_setter` executes the same cases via SetMember in interpreted,
attached and automatic JIT modes and asserts native entries. The allocator sweep
`object_property_case_allocation` covers flags/delete/own data updates, failures
through success, unchanged values/flags on OOM and restored root/setter metadata.
Watchers, definition APIs, duplicate-order semantics, complete scope/name rules
and folded traversal work accounting remain open. All nine stages remain open.

Validation v0.425: Linux minimal 845/845; AGG, NO_OS_PORTS and heap/global
ASan+UBSan 7/7 each. Native entry assertions run on available Linux x64; the
NO_OS_PORTS configuration is a Linux-hosted build, not another OS run. Generated
mode launchers used; stack ASan/LeakSanitizer disabled. All stages remain open.

## v0.426 — own membership through super

Loaded super facade own-membership now delegates to its retained receiver,
matching pinned script_object.rs has_own_property. The query does not inspect
private facade metadata or enumerate/read parent properties. It preserves the
receiver's SWF casing. Host facade behavior remains unchanged. Nested/malformed
receiver-facade chains have a bounded iterative traversal instead of recursive
C++ calls; malformed cycles return FVM_ERROR.

`object_super_membership` failed against v0.425 and covers loaded SWF5–8 plus
host, direct C and borrowed Object.hasOwnProperty.call, counted NUL names,
exact/folded spelling, inherited/missing properties and a throwing AVM1 getter
which membership never invokes. Real GC, malformed-cycle rejection/recovery and
queue transfer to another loaded context are checked. The destination repeats
the queries after collecting. Full super enumeration/flags/deletion/definitions,
independent Flash validation, other transports/platforms and all stages remain
open. This is a query-only compatibility change.

Validation v0.426: Linux minimal 846/846; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack
ASan/LeakSanitizer disabled. No independent Flash or other OS certification.

## v0.427 — watcher name selection

Loaded SWF4–6 watch replacement, dispatch, raw old-value lookup and unwatch use
Flash name equivalence. The callback receives the requested counted spelling,
not the stored property spelling. Own-property selection now happens after the
watcher returns, so callback mutation is resolved against the current table.
Host/SWF7–8 keep exact watcher names. Getter-free old-value lookup still preserves
virtual stored data, numeric slots and lazy native methods.

`object_property_case_watch` failed on v0.426's callback-name assertion. It checks
SWF4–8/host, NUL names, old/new/userData arguments, single-record replacement,
case-varied unwatch, self-removal followed by real GC, errors/counting exceptions
and running/root cleanup. Primary reference: pinned script_object.rs watch,
unwatch and call_watcher, which passes the requested name to Watcher.call.
Exception-side writeback, watcher recursion identity/collision details, complete
movie-clip watch behavior and budgeted folded traversal remain open. The existing
exception policy is retained here; no complete stage is claimed.

Validation v0.427: Linux minimal 847/847; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS execution and all-stage acceptance remain open.

## v0.428 — case-aware virtual replacement

Loaded SWF4–6 addProperty selects an existing own stored spelling before checking
attributes and replacing its descriptor. Stored data and flags survive replacement;
a variant does not create a second virtual property beside the original data.
Host/SWF7–8 preserve distinct names. The existing null-setter and rollback rules
are retained. This follows pinned script_object.rs add_property_with_case.

`object_property_case_definition` first failed on v0.427. It checks SWF4–8/host,
NUL names, retained flags/data, a collecting script getter, no premature getter
execution and writable retained data with a null setter. An allocation sweep runs
through failure to successful definition, checking previous values/flags and roots.
`jit_property_case_watch` additionally checks requested watcher names, collecting
self-removal and errors via interpreted/attached/automatic native SetMember, with
actual native-entry assertions. Native C definitions, collisions, watcher throw
writeback, complete property semantics and all stages remain open.

Validation v0.428: Linux minimal 850/850; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 8/8 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. No other OS or complete-stage certification.
