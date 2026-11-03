# Native owner-scope error checkpoints (runtime 0.62.0)

This step connects registered native cleanup to the actual hosted Proc error
labels. It does not change the retained Proc prefix, REG layout, Heap/Type/Jit,
file IDs or exclusive managed NativeWorld admission. Extended-up native storage
and DVM_OwnerScope grow; rebuild native clients against matching headers/sources.
The new metadata contains no managed pointers or GC roots.

## Before the jump

The runtime generates a reviewed dat.h view from the selected upstream header.
Only waserror/poperror macros change, with drift checks. waserror captures a
native cleanup generation and THEN executes the original ossetjmp in its caller.
No function returns a saved setjmp frame. Every native error label has an
owner-local checkpoint alongside existing Proc labels, outside REG snapshots.

Actual nexterror calls dvm_native_error_unwind BEFORE siglongjmp. Newer registered
scopes close LIFO while their native C frames and caller-provided entries are
still alive. Close unlinks/clears entries before drops; final drops may free
native objects. The native error message remains in the current owner's buffer.
An inner catch retains scopes opened before its checkpoint. A rethrow unwinds
to the next enclosing checkpoint, preserving still-older scopes.

A checkpoint is a monotonically issued generation, not a pointer to a scope
that may already have closed or reused the same address. Generation exhaustion
refuses registration rather than wrapping. Registration/unwind are owner-local;
no registry mutex or per-payload locality test is added by this mechanism.
The callbacks themselves still obey their reference-count backend's rules.

## Normal return and failures

Normal poperror refuses if newer scopes are still registered. The generated
macro treats that refusal as fatal. It checks scalar live-generation metadata;
it does not attempt late cleanup through a callback's already-returned C stack.
Native callbacks must explicitly close their scopes before NORMAL return.
A raw return outside a protected boundary cannot be made safe automatically:
registered scope/entry storage must always outlive registration.

Cleanup callbacks must return normally and unlink their own record. They cannot
enter the VM, longjmp, switch native context or acquire managed payload rights.
New cleanup registration and recursive unwind/error push refuse during cleanup;
TLS unbind refuses even when the last record is being retired. Dispatcher captures
callback/argument/serial before calling and never dereferences the old record
after return, allowing a callback to unlink and free its own native storage.

Held object guards refuse unwind before any callback. Native code must release
its guards before raising a VM error; this step does not auto-unlock them. A
callback failure or lack of progress may leave partial cleanup: actual runtime
terminates rather than jumping through a partially cleaned continuation.
No transaction, arbitrary asynchronous OS cancellation or signal-handler cleanup
is provided. Only registrations covered by the native host contract participate.

Checkpoint cleanup concerns scopes opened after the mark. A reference inserted
into an older persistent scope remains there; this API is not rollback of every
mutation since a mark. Copying/moving registered scope metadata is invalid.
Foreign observation requires stable, synchronized metadata, never concurrent
retirement of a borrowed registration or context.

## Exact validation boundary

The native fixture generates the same dat.h macros, compiles actual context,
register/error/owner-scope adapters and extracts the unchanged hosted error,
nexterror, owner_error and kprotect bodies. It uses retained Proc declarations
and real OS threads/ports. Per backend, eight independent owners perform 640
nested error transfers and 960 final native message frees. It checks inner catch,
rethrow, outer preservation, message text, balanced labels, normal-pop refusal,
held-guard refusal, callback reentry, closed/reused scope generations and an
unlinking callback that frees its own record. Heap-resident episode metadata
avoids inspecting indeterminate modified automatic locals after setjmp.

Native-selected/forced-lock reference backends and ten isolated removal controls
run; controls must exit with semantic status 10, never compiler/signal/sanitizer/
timeout failure. Normal, nonrecovering UBSan and focused TSan are separate gates.
The fixture does not instantiate or collect a managed heap. Full runtime exception,
worker/RC/pool checks separately cover integration of the generated macros.

Still pending: managed root registration/pinning, process (not merely native C
frame) ownership, actual managed scope drops under the right heap phase, all C/JIT
count/mark paths, allocator/object protection and parallel managed admission.
Native scope cleanup is not proof that those paths are now safe concurrently.
