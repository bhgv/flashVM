# AVM1 active-frame garbage-collection roots

The collector now traces the operand stack, lexical local bindings and receiver of each active AVM1 execution frame. Frames are linked to the owning `FVM_Context` and removed using C++98 stack unwinding on all exit paths. Nested script calls contribute separate frames.

The existing global-object and explicitly pinned-object roots remain active. Cyclic references among unreachable objects are still collected by mark-and-sweep; the finalization phase re-marks after callbacks to account for resurrection.

## Safety and limitations

This is **not yet a complete production GC**. In particular, arbitrary native callbacks, temporary C API values, results held by callers and references in native extensions need an explicit root discipline. Native callers should pin object values before requesting collection. Automatic collection inside the AVM1 dispatch loop remains disabled until native callback and temporary-value rooting is audited. String allocations are managed separately from object GC. The ActionScript `gc` object is not yet implemented.

Collection is context-local and must run on the context's owning thread. Do not call `fvm_gc_collect` concurrently with the same context's execution. The public C ABI is unchanged by this stage.
