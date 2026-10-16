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
