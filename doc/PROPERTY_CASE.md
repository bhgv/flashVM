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

## v0.429 — rooted C definitions and stored names

Rooted native method/accessor definition selects the existing own spelling in
loaded SWF4–6 before applying the existing conflict policy. Replacing a method or
accessor no longer introduces a hidden case-equivalent duplicate; a data/accessor
conflict which fails for exact spelling also fails for its equivalent name.
Host/SWF7–8 keep distinct names. C89 signatures and ABI are unchanged.

`object_property_case_native` reproduced v0.428's duplicate-method failure. It
checks SWF4–8/host, counted NUL names, replacement callbacks/capture roots, method
and accessor table identity, rejected data conflicts, actual GC and invocation of
a previously pinned Function which retains its old callback/capture. Native API
policy is an embedding contract aligned with loaded lookup; it is not an Adobe C
API claim. Stale unexported method caches, full callback lifetime/collision rules,
watcher exception semantics and all stages remain open.

Validation v0.429: Linux minimal 851/851; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS and complete-stage acceptance remain open.

## v0.431 — raw Array length in case-varied watchers

Getter-free watcher old-value lookup recognizes Array resource length with the
same counted Flash case rule as ordinary loaded names. SWF4–6 writing LENGTH
while watching length receives old length 3, requested name LENGTH and new value
5; returning 4 resizes the Array to four elements. SWF7–8 preserve distinct names;
host native-length watch policy is unchanged. NUL suffixes remain distinct.

`array_case_watch_length` failed on v0.430's old-value assertion, then checks
host/SWF4–8, callback arguments, actual getter-free resource value, real GC,
resize and NUL-suffix isolation. `jit_array_case_watch_length` repeats resize via
interpreted, attached and automatic native SetMember with native-entry assertions.
This supplements the pinned watcher/property-name reference; complete Array
accessor overrides, exception writeback, budgets and all stages remain open.

Validation v0.431: Linux minimal 855/855; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS execution and complete-stage acceptance open.

## v0.435 — borrowed definitions on super

Loaded borrowed Object.addProperty and Object.watch acknowledge valid arguments
on a super facade without storing descriptors/watchers on it. Object.unwatch
returns false for that facade. Name coercion and argument validation still run,
including errors from a coercion callback. Loaded watch rejects a super facade
as its callback: super has direct-call machinery, not ordinary Function identity.
Host behavior is retained. Direct super.method calls still bind the underlying
receiver; this change concerns explicitly borrowed methods with this = super.

`object_super_definitions` reproduced v0.434's stored accessor. It checks SWF5–8
and host, counted NUL names, accepted/rejected arguments, exact coercion counts,
coercion exceptions, descriptor/watcher absence, callback rejection and GC.
Primary references: pinned globals/object.rs add_property/watch/unwatch and
script_object.rs add_property_with_case/watch/unwatch, which decline mutation
of NativeObject::Super. Full facade reflection, native definition APIs, accessor
callback projection and all roadmap stages remain open.

Validation v0.435: Linux minimal 862/862; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS execution and all-stage acceptance open.

## v0.436 — super is not a virtual Function callback

Loaded script-property getter/setter dispatch now excludes super facades from
ordinary Function callbacks. A non-function getter uses retained property data;
a non-function setter has no executable side effect and writable data still
updates. The facade remains valid as an Object argument to addProperty. Host
behavior and explicit direct superclass invocation remain unchanged. Pinned
script_object.rs call_getter/call_setter require as_function before execution;
Property retains data independently of getter/setter objects.

`object_super_accessors` first failed on v0.435's retained-data read. Across
host/SWF5–8, a constructor side effect detects accidental parent execution during
get/set, real GC checks descriptor-held facade graphs, direct super still executes
the parent, and queue transfer to another context repeats the retained-data and
side-effect checks after GC. `jit_object_super_accessors` executes GetMember and
SetMember in interpreter, attached and automatic JIT, asserting native entries.
This covers ordinary descriptor dispatch, not complete super/reflection or RPC
semantics. Callback/resource equivalence and all nine stages remain open.

Validation v0.436: Linux minimal 864/864; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS and all-stage acceptance remain open.

## v0.437 — setter replacement spelling

After an own script setter returns, loaded SWF4–6 reselect the current own
property spelling before checking readonly flags and committing stored data.
A callback may delete `Slot` and recreate `sLOT` as ordinary data or a virtual
property; the original input updates that replacement without invoking its
setter again. A readonly replacement keeps its data. Host and SWF7–8 keep
exact spelling. This follows pinned Ruffle script_object.rs set_local's
case-aware property entry lookup after executing the setter.

object_setter_case_replacement reproduced the old duplicate-slot write, then
checks all three replacements, host/SWF4–8, a counted NUL string, collection
inside the setter and traced-string collection after the write.
jit_object_setter_case_replacement repeats loaded cases through interpreted,
explicit and automatic JIT SetMember with actual native entries. Native
accessor replacement, broader collisions and all nine stages remain open.

Validation v0.437: Linux minimal 866/866; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers were applied;
stack ASan and LeakSanitizer remain disabled. Foreign OS execution is open.

## v0.438 — borrowed Array raw length casing

Loaded borrowed Array methods now find own raw `LENGTH` data/virtual storage
as `length` in SWF4–6. Raw writes select the existing own spelling before
readonly checks and descriptor-data updates. No getter/setter is executed.
SWF7–8 remain case-sensitive. This follows pinned script_object.rs
length/get_data/set_length/set_data and globals/array.rs push/splice.

array_borrowed_length_case reproduces the former empty slice with `LENGTH=2`,
and checks slice/push/splice for ordinary, virtual and readonly virtual data,
SWF4–8, retained spelling, NUL-suffixed separate names and callback counts.
jit_array_borrowed_length_case repeats the matrix with real native CallMethod
entries. Broader Array behavior, all resources and all nine stages remain open.

Validation v0.438: Linux minimal 868/868; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.439 — borrowed Array methods on super

The loaded raw own-data write bridge ignores super facades. In particular,
Array.push/unshift/splice borrowed with Function.call no longer create private
indices or a length slot on escaped super. Return values and direct superclass
calls retain their contracts; host writes are unchanged. Pinned
script_object.rs set_element/set_length explicitly return without storing on
NativeObject::Super. This correction concerns Array raw writes, not a claim
about every ordinary super assignment or C extension operation.

array_borrowed_super reproduced the facade mutation, then runs push, unshift,
splice, slice, pop, shift and reverse successively for SWF5–8. It checks return
values, unchanged raw storage and receiver state, actual GC after each method,
and clean call/root state. jit_array_borrowed_super repeats all calls in the
interpreter and two native modes, asserting actual entries. Full super/Array,
transport and all nine acceptance stages remain open.

Validation v0.439: Linux minimal 870/870; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.440 — registerClass rejects super constructors

Loaded Object.registerClass rejects a super facade as a constructor before
coercing the linkage name. A callable parent interface is not a Function
constructor. Invalid registration leaves the existing constructor intact.
Host acceptance and null/undefined unregistration remain unchanged. Pinned
globals/object.rs register_class validates as_function before string coercion.

object_register_class_super reproduces the old acceptance/name callback, then
checks SWF5–8 and host, NUL linkage keys, callback counts, suppression of a
throwing name conversion for invalid constructors, propagation for valid
constructors, existing registration preservation, successful retry and null
unregistration followed by GC. jit_object_register_class_super repeats
registration through interpreted/explicit/automatic CallMethod with real
native entries. General constructor semantics and all stages remain open.

Validation v0.440: Linux minimal 872/872; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.441 — loaded super TypeOf

The shared TypeOf helper reports `object` for a loaded super facade even though
it supports explicit parent invocation. Real Function objects still report
`function`; ordinary instances report `object`. Host behavior is retained.
The allocation-free classification follows pinned avm1/value.rs type_of,
which recognizes Functions through as_function rather than generic callability.

object_super_typeof reproduces the previous `function` result, then checks
host/SWF5–8, in-place C helper output, actual Functions, instances, collection
and repeat, no constructor side effect, and preservation of direct super
invocation. jit_object_super_typeof repeats TypeOf in interpreted, explicit
and automatic JIT execution with real native entries. Other super reflection,
conversion and all nine stages remain open.

Validation v0.441: Linux minimal 874/874; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.442 — super string fallback

Loaded ToString now falls back to `[type Object]` for a super facade when its
parent-visible toString returns a non-string. Generic call support does not
make the facade a Function; actual Functions keep `[type Function]`. String
returns (including NUL) and thrown values pass through unchanged. Host is
unchanged. Pinned avm1/value.rs coerce_to_string checks as_function only after
executing the special toString method. Receiver/owner dispatch is a separate
remaining concern; this change only corrects the fallback classification.

object_super_string reproduced the previous Function fallback. It checks
SWF5–8, number/undefined/null/string/exception returns, callback count, in-place
C helper, Function/instance controls, GC and repeatability of direct parent
invocation. jit_object_super_string repeats ToString in three execution modes
with actual native entries. Full conversions and all stages remain open.

Validation v0.442: Linux minimal 876/876; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.443 — special parent conversion receiver and owner

Loaded numeric/default/string conversion of super now uses parent-method
dispatch with the retained original receiver and the selected defining owner.
Missing or noncallable conversion members yield undefined; real callee errors
propagate. Owner selection uses versioned spelling and is rooted before a
getter can detach it. Existing explicit-call error behavior and host conversion
are retained. Pinned object.rs and super_object.rs call_method distinguish
Special calls and execute against super.this(), with parent lookup depth.

object_super_coercion_receiver reproduced the previous facade receiver. It
checks ToNumber, ToString and default primitive conversion in SWF5–8, folded
SWF5–6 names, primitive/noncallable Object members, number/NUL string results,
exception/plain error, collection inside callbacks and self-deletion plus
prototype detachment. Callback receiver/owner and cleanup are asserted.
jit_object_super_coercion_receiver repeats ToNumber/ToString/Add2 in three
modes with actual native entries. Broad resolution/owner mutation, failure
boundaries and all nine stages remain open.

Validation v0.443: Linux minimal 878/878; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 4/4 each. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. Foreign OS execution and all stages remain open.

## v0.444 — allocation-free invocation identity roots

FVM_CallScope now retains saved/active constructor, function and owner through
six inline root frames. Publishing the complete chain and restoring the
previous identity no longer allocate a temporary vector. Nested calls and
unwinding preserve the caller's root head. This is an implementation/GC
change, not an expansion of SWF compatibility.

super_conversion_allocation reproduced failure to enter a call scope with all
new/new[] denied. It now enters/exits and unwinds a nested scope under that
restriction, collects with all six identities live, and verifies reclamation
after detachment. It also sweeps every new/new[] boundary up to first completed
ToNumber/ToString/Add2 success or thrown value across SWF5–8, using fresh
contexts. Failures preserve aliased output, reset recursion/invocation roots,
retain any pending thrown payload and permit GC/retry. These are C++
allocation boundaries, not every OS/C allocator or full concurrent GC proof.
All nine stages remain open.

Validation v0.444: Linux minimal 879/879; minimal/AGG/Linux-hosted NO_OS_PORTS/
heap-global ASan+UBSan targeted checks 5/5 each after dependency-cache repair.
The first sanitizer run exposed a stale function.cpp object using the old
CallScope layout; regenerating dependency caches rebuilt it and the check
passed. Executable-mode launchers applied; stack ASan/LeakSanitizer disabled.
Foreign OS execution and all stages remain open.
