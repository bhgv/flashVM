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
