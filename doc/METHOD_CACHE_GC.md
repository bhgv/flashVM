# Native method cache roots

v0.430 invalidates stale lazy Function wrappers after a successful native method
replacement, method deletion, ordinary data overwrite or native accessor
replacement. Method-table entries shadowed by published data/accessors are also
removed. Cleanup happens after fallible publication, preserving the old entry and
wrapper on OOM. Identical callback/userdata/root definitions keep Function identity.

[GC:] A retained or pinned Function owns its own callback capture. Removing the
owner's cache does not change that exported Function or its callable behavior.
An unexported wrapper no longer keeps its obsolete capture alive solely through
a pinned owner object. This applies to host and loaded contexts; existing naming,
readonly and deletion checks still run first. No public ABI change.

`gc_method_cache` first failed on v0.429. Across host/SWF4–8 it tests all four
operations with and without a pinned old Function, identical-definition identity,
actual collection, safe live-registry checks before dereferencing, old callback
invocation while pinned and release after unpin. `gc_method_cache_allocation`
sweeps every failure through success in host/SWF6/SWF8, checking old value,
Function identity and capture retention on OOM, then release on success.

This is not complete callback or GC certification. Global registration caches,
other wrapper/resource subsystems, finalizer resurrection and concurrent RPC
lifetimes require their own acceptance. All nine roadmap stages remain open.

Validation v0.430: Linux minimal 853/853; AGG, Linux-hosted NO_OS_PORTS and
heap/global ASan+UBSan 6/6 each. Generated mode launchers used; stack ASan and
LeakSanitizer disabled. Other OS and complete-stage acceptance remain open.
