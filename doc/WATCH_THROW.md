# Thrown property watchers

v0.432 completes loaded Object/Array assignment after a script watcher throws:
the assigned value becomes undefined. A successful own write then returns the
original exception; readonly data stays unchanged and still returns the exception.
Own virtual setters run with undefined and the original watcher exception wins
over a later setter exception. Inherited virtual dispatch returns the setter's
result, consuming the watcher exception when the setter succeeds; its own throw
is propagated. Host watcher failure policy is unchanged.

The source basis is Ruffle commit 2372a4e148a67d68efd796f4477b3cddd4f880cd:
[object.rs set_internal](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object.rs)
and [script_object.rs call_watcher](https://github.com/ruffle-rs/ruffle/blob/2372a4e148a67d68efd796f4477b3cddd4f880cd/core/src/avm1/object/script_object.rs).
The latter changes a thrown watch value to undefined; set_internal continues
assignment and treats inherited setters separately from own set_local.

[GC:] FVM_WatchWriteScope saves the thrown graph before clearing pending state.
Its explicit root frame protects counted strings, objects and cycles through
setter execution and collection. Destruction restores pending ownership before
unlinking the saved frame. It unwinds before the enclosing lookup-owner root.
Saving may allocate; failure before suspension leaves the original pending
exception intact. Ordinary non-throwing writes allocate no saved-value vector.

`object_watch_throw` first failed on v0.431. Host/SWF4–8 cover named data,
Object/Array numeric slots, readonly data, own/inherited script accessors, setter
success/throw and error priority. A self-cyclic object thrown by the watcher
survives a real collecting setter; pending state is suspended inside that setter.
Existing counted-string/interpreter/native watch fixtures now expect undefined
writeback for loaded throws. Raw runtime FVM_ERROR and allocation failures retain
fail-fast handling; their equivalence to reference errors is not certified.
Custom native length/resource setters, metatables, recursion limits and complete
watcher/SWF compatibility require further checks. All nine stages remain open.

Validation v0.432: Linux minimal 856/856; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other operating systems and all stages remain open.

## v0.433 — Array growth excludes inherited virtual dispatch

An indexed loaded Array write now resolves whether a virtual property is inherited
before growing length. Inherited setters, including getter-only virtual properties,
do not enter Array.set_local, so they do not grow the receiver. Own writes retain
growth before readonly checks and own setter invocation. This refines the earlier
blanket growth-before-setter policy. Pinned object.rs set_internal dispatches
inherited setters before script_object.rs set_local performs native Array growth.

`array_inherited_setter_length` failed on v0.432: an inherited index 5 setter saw
length 6 instead of 0. Host/SWF4–8 checks own/inherited dispatch, real GC, setter
throws, thrown watchers and priority/consumption rules; inherited writes leave no
own slot. The native fixture repeats all loaded cases in interpreter/attached/
automatic JIT modes, asserting native entry counts and length inside the setter.
Full Array side effects, resource-specific errors, malformed graphs and work
accounting remain open. No complete stage or platform certification is implied.

Validation v0.433: Linux minimal 858/858; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 5/5 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS and all-stage acceptance remain open.

## v0.434 — getter-only dispatch and deferred-error acceptance

A loaded named write through an inherited native getter-only virtual property
now consumes the watcher exception, as inherited call_setter with no setter does.
It leaves no own value and does not execute the getter. Own getter-only properties
still return the watcher exception. Numeric dispatch already followed this rule.
`object_watch_getter_only` reproduced v0.433's named-path mismatch and checks
SWF4–8, own/inherited/numeric/named variants, counted exceptions and real watch GC.

`jit_object_watch_throw` executes the complete data/readonly/own/inherited setter
matrix with cyclic thrown objects and collecting setters in interpreter, attached
and automatic JIT, asserting actual native entry counts. Allocation failures are
swept until the first completed throw-writeback in `object_watch_throw_allocation`:
host/SWF4–8 retain old data on OOM, preserve an already pending exception when
suspension or subsequent preparation fails, clear state and retry. Saved exception
payload and root/watch/setter cleanup are checked after every boundary. These
checks cover own raw writes, not every allocator failure inside a setter.
Other error mappings, resource setters, recursion/collision limits and all nine
roadmap stages remain open. Native-accessor policy remains an embedding contract.

Validation v0.434: Linux minimal 861/861; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated native-mode launchers used; stack ASan
and LeakSanitizer disabled. Other OS execution and all-stage acceptance open.
