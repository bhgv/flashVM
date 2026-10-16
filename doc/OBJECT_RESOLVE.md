# Loaded Object.__resolve — v0.450

For an ordinary loaded property read which exhausts the prototype chain,
FlashVM searches raw own `__resolve` data along the original lookup chain.
SWF4–6 use Flash case equivalence; SWF7–8 use exact names. Primitive resolver
entries are skipped. The first Object entry stops the search: an ordinary
noncallable Object produces undefined, while a Function (or callable super
facade) is invoked with the original receiver and one counted requested name.
The resolver property's getter is never invoked; script virtual properties
contribute their retained data. Native methods contribute their Function value.

Existing properties, including stored undefined, stop ordinary lookup before
resolution. Numeric Object/Array property fallback shares this path. Own
membership queries do not invoke a resolver. Missing parent property reads
search the parent chain and retain original this; the API input super facade
also remains rooted while the resolver mutates the graph and collects.

Special numeric/default/string conversions skip `__resolve` for missing
valueOf/toString. A separate recursion counter bounds both native and script
resolver callbacks to 64; it restores on errors/unwinding. Existing prototype
bounds and native barriers apply. Resolver results precede the FlashVM
metatable __index extension; no Object resolver preserves the old extension
fallback. Host mode is unchanged.

The behavior is based on pinned Ruffle
[object.rs search_prototype/find_resolve_method](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs),
[script_object.rs get_local_stored](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs),
and [value.rs special conversions](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/value.rs).
The depth/error mapping, metatable precedence and C status codes are explicit
FlashVM policies, not a claim of complete player equivalence.

object_resolve reproduces the missing hook and checks host/SWF4–8, exact/mixed
resolver names, own/inherited/primitive/noncallable/virtual/native entries,
counted NUL/Unicode and indexed reads, callback arguments/receiver, GC, owned
returned strings, exceptions/plain errors and membership/stored-undefined
suppression. jit_object_resolve repeats GetMember in three modes with actual
native entries. object_resolve_special checks conversion exclusion, bounded
native recursion, malformed prototype cycles and recovery. object_resolve_super
checks original this, self-deletion/detachment, unpinned API-input facade
survival and exception cleanup.

Direct C/super method resolver dispatch is covered by v0.451 below.
Variable-scope resolution, transport/OOM coverage, broader resources and all
nine stages remain open. No new original SWF corpus pass is claimed.

Validation v0.450: complete Linux minimal 890/890; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 6/6 targeted checks each. Added metatable precedence
cases were then rerun in all four profiles. Dependency indexes were refreshed
for the private context layout change. Executable-mode launchers applied;
stack ASan/LeakSanitizer disabled. All stages/foreign OS runs remain open.

## v0.451 — resolver-returned method calls

Loaded direct C missing-method calls and explicit super methods now use the
raw resolver search. A returned Function runs with the original receiver,
snapshotted original arguments and no defining property owner. C missing or
nonfunction values retain UNSUPPORTED; AVM1 parent calls return undefined for
those values. Actual resolver/callee errors, including UNSUPPORTED, propagate
without being confused with absence. Special conversions still skip resolver;
host dispatch and global native fallback without a resolver are retained.
Ordinary Lua-style callable Objects are not AVM1 Functions.

object_resolve_call reproduced the unsupported direct C call, and a separate
control caught the super/nonfunction callable distinction. Host/SWF4–8 check
ordinary/parent receivers, C/AVM1 APIs, Function/scalar/plain Object/Lua callable/
absent/throwing/unsupported results, original counted method names, cyclic and
NUL arguments, self-deletion/prototype detachment, caller-array mutation, GC
and call/root/recursion cleanup. jit_object_resolve_call repeats CallMethod in
three modes with actual native entries and checks callbacks are not replayed
when they fail. Variable scope, transport/OOM and all stages remain open.

Validation v0.451: complete Linux minimal 892/892; AGG, Linux-hosted NO_OS_PORTS
and heap/global ASan+UBSan 5/5 targeted checks each. Executable-mode launchers
applied; stack ASan/LeakSanitizer disabled. All stages/foreign OS runs remain open.

## v0.452 — resolver OOM publication

The resolver invokes into a private result and publishes it only on OK. In
particular, failure while snapshotting a returned view of the temporary counted
request name leaves the public output undefined instead of a dangling string.
Exceptions remain in the context; error values are not published as successful
results. Registration/callback side effects are not rolled back.

object_resolve_allocation reproduced the unsafe output under new/new[] failure.
Fresh-context sweeps cover every boundary up to first completed OK/exception in
SWF4–8, named NUL/indexed reads, native Function/script body/native method
resolvers, cyclic captured exception payloads, root/recursion/script-depth
cleanup, pending payload preservation, GC/retry and retained successful strings.
These are C++ allocation boundaries, not every C/OS allocator or a full
concurrency proof. Variable scope, graph transport and all stages remain open.

Validation v0.452: complete Linux minimal 893/893 and AGG 912/912;
AGG/Linux-hosted NO_OS_PORTS/heap-global ASan+UBSan targeted checks 6/6 each.
The first full AGG run found one old jit_array_sort_on_case object predating
the private context layout. Removing that stale object forced recompilation;
the full 912-case rerun passed. Executable-mode launchers applied; stack
ASan/LeakSanitizer disabled. All stages/foreign OS runs remain open.

## v0.453 — resolver status and original native interfaces

Resolver discovery now reports presence separately from callback status. An
actual FVM_NOT_FOUND from a resolver, returned method or revoked RPC interface
propagates unchanged; it cannot masquerade as an absent hook or trigger fallback.
Existing interpreter/JIT tests cover resolver and callee NOT_FOUND, including
super dispatch and callback counts. object_resolve_rpc checks SWF6/8 with two
independent runtimes, source context served exclusively by the broker thread,
original native callback/userdata, cyclic receivers, counted NUL/Unicode names,
exceptions, unsupported/not-found errors, recovery and revocation over one port.
NO_OS_PORTS skips the socket-specific test; it is not network evidence there.

The C89 property_resolve embedding tutorial returns an original C Function for
an absent sum method, checks GC capture ownership and prints sum = 42. Its RU/EN
READMEs explain building/running and link the separate process/thread/LAN RPC
tutorial. This does not establish full distributed graph or variable resolution
semantics. All nine stages and foreign OS acceptance remain open.

Validation v0.453: complete Linux minimal 895/895; AGG, Linux-hosted
NO_OS_PORTS and heap/global ASan+UBSan 7/7 selected checks each. The socket
case is skipped in NO_OS_PORTS. Executable-mode launchers applied; stack ASan
and LeakSanitizer disabled. Other OS runs and all stages remain open.
