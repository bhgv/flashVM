# Native MovieClip content unloading

[Русский](MOVIE_UNLOAD.ru.md) · [Movie sources](PLAYER_SOURCES.md) ·
[Executable C89 tutorial](../examples/embedding/movieclip/README.md) ·
[Roadmap status](ROADMAP_STATUS.md).

## Atomic content clearing

`fvm_movieclip_unload(context, clip)` clears a same-context native instance,
including its drawing, frame actions, imported dictionary and children. It retains
the instance identity, prototype, placement, own properties, lockroot state and
retained clip handlers. The replacement has one stopped empty frame. A parent
binding still names the same instance. Matching bindings to old children are
removed; externally retained children become detached and keep their source base.
The target's source provenance remains available for later relative resources.

Call on the VM owner thread at a safe point, outside AVM1 execution, native
callbacks, sound delivery, GC and teardown. The context and target must remain
alive; pin clips retained across calls that can collect. No constructor, property
setter, frame action or unload callback is invoked. Repeated clearing is allowed.
All allocations and validation precede publication. [`FVM_OK`](../include/flashvm/flashvm.h#L42) commits the complete
replacement; [`FVM_OUT_OF_MEMORY`](../include/flashvm/flashvm.h#L44) leaves the old content and child bindings intact.
Null/foreign objects, class prototypes, internal static shapes and invalid lifecycle
states return [`FVM_INVALID_ARGUMENT`](../include/flashvm/flashvm.h#L45). Other malformed private state can return
its validation error; unexpected implementation exceptions become [`FVM_ERROR`](../include/flashvm/flashvm.h#L43).
The operation requests a stage update after publication and borrows its arguments.

## Native attachment query

`fvm_movieclip_attached_to(context, clip, ancestor, out)` follows physical native
parent edges, without script getters, `_parent` overrides or `_lockroot` semantics.
All pointers are required and both objects must be same-context instances. A live
instance includes itself. Any detached node on the selected chain yields zero;
a chain ending before the ancestor also yields zero. Success writes zero or one.
Invalid state, allocation failure and chains exceeding
[`FVM_MOVIECLIP_QUERY_MAX_DEPTH`](../include/flashvm/movieclip.h#L117) preserve the caller's output. The depth bound
also rejects cycles. Use on the owner thread outside GC/teardown, retaining both
borrowed objects through the call. This is a snapshot, not a retained subscription.

## Integration boundaries

This C operation is a foundation for host-managed unloading. It does not install
ActionScript `unloadMovie`, cancel network requests, dispatch `onUnload`, stop an
independently owned audio device or implement the full Flash loader lifecycle.
Since v0.692 the desktop player also installs the bounded script interface below.
Other embeddings still own outstanding transfers and their pinned targets.
An embedding must coordinate these separately before accepting subsequent delivery;
calling this API alone does not prevent a later request from replacing the clip.
No public host options or callback structure layout changes.

## Verification

`movieclip_unload` checks pixels before/after clearing, retained identity,
properties, transforms, handlers and source bases, detached children, subsequent
GC/ticks, repeated clearing and forbidden constructor/prototype callbacks.
`movieclip_unload_allocation` injects failures at successive allocation boundaries
and checks unchanged old state after every failure. Existing content-replacement
and replacement-allocation fixtures remain part of the focused regression set.

```sh
cmake --build build --target flashvm_tests_movieclip_unload flashvm_tests_movieclip_unload_allocation flashvm_examples_embedding_movieclip_main
ctest --test-dir build -R 'movieclip_(loading|unload)|examples_embedding_movieclip_main' --output-on-failure
```

The example uses only the public C API and needs no SDL, network or SWF compiler.
Use the normal [native/cross-build guide](DESKTOP_FRONTEND.md) for toolchain setup;
foreign linking is not evidence of execution on that processor or operating system.
## Desktop script interface since v0.692

The optional player accepts `unloadMovie("child")`, `unloadMovie(child)` and
`child.unloadMovie()`. String targets resolve in the calling AVM1 scope; object
arguments retain the exact native identity. Only attached child instances are
supported. Root/_level0 unloading returns UNSUPPORTED: its audio, source and window
lifecycle is not implemented by this child operation. No onUnload event is generated.

The script callback pins and queues the operation. The next owner safe point
clears contents before it polls an active transfer. An earlier active child/form
request targeting that clip or its physical descendants is cancelled and joined
before its target pin is released. Earlier queued requests in that subtree are
dropped without I/O. Requests to unrelated instances retain FIFO order. Requests
queued after the unload may reload the retained instance. No worker calls the VM.

Unloads have a separate queue bound of 1024 and do not consume fetch budget;
`--loads 1` still permits local unloading. All subtree decisions and core content
allocations precede cancellation; a failed clear does not commit those cancellations.
Normal close/error cleanup releases queued unload pins together with fetch pins.
The C host interface and CLI select the same player implementation. This remains
safe-point deferral, not synchronous Flash timing or complete loader event support.

`unload_loading.py` executes real local and HTTP SWFs in interpreter/JIT, checks
string/object/method forms, retained properties and identity, descendant deletion,
active/queued cancellation, unrelated delivery, later reload and a wrong-reply
negative control. Network-free builds still execute its local cases.

<!-- source-links:start -->
## Исходники / Source declarations

Ссылки ведут к объявлениям/определениям в текущем дереве исходников;
номера строк обновляются скриптом `scripts/link_documentation_sources.py`.
Source line links describe the current checkout, including in historical notes.

| Символ / Symbol | Вид / Kind | Исходник / Source |
| --- | --- | --- |
| `FVM_ERROR` | enum constant | [include/flashvm/flashvm.h:43](../include/flashvm/flashvm.h#L43) |
| `FVM_INVALID_ARGUMENT` | enum constant | [include/flashvm/flashvm.h:45](../include/flashvm/flashvm.h#L45) |
| `fvm_movieclip_attached_to` | function | [include/flashvm/movieclip.h:122](../include/flashvm/movieclip.h#L122) |
| `FVM_MOVIECLIP_QUERY_MAX_DEPTH` | macro / define | [include/flashvm/movieclip.h:117](../include/flashvm/movieclip.h#L117) |
| `fvm_movieclip_unload` | function | [include/flashvm/movieclip.h:132](../include/flashvm/movieclip.h#L132) |
| `FVM_OK` | enum constant | [include/flashvm/flashvm.h:42](../include/flashvm/flashvm.h#L42) |
| `FVM_OUT_OF_MEMORY` | enum constant | [include/flashvm/flashvm.h:44](../include/flashvm/flashvm.h#L44) |
<!-- source-links:end -->
