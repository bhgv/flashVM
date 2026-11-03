# Managed lifetime across native workers

The complete, commented C example runs inside the actual hosted Dis runtime:
[managed_owner_probe.c](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/managed_owner_probe.c).
Its native receiver, native-error callback and root/collector orchestration are
separate functions; managed root/retirement machinery is a separate module:
[managed_owner.c](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/managed_owner.c).

Build the normal disvm-runtime or disvm-editor workspace product, then run its
binary with --gc-worker --managed-owner-probe --stats and an existing ordinary
native-cell Dis application. The probe executes before the normal application;
it is an optional diagnostic, not an alternative VM or collector. --gc-inline
refuses this probe because no native Fast retirement service exists.

From a configured runtime build, the automated driver also compiles a small
ordinary Limbo application and tests both dispatch/poll selections:

```sh
python3 disVM/runtime/test_managed_owner.py \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module ide/assets/fonts/NotoSansMono-Regular.ttf
```

Use your own build directory if different. For a CMake Makefiles build, an optional
root-removal control reuses its actual compile/link metadata and changes only a
temporary replacement adapter/object/binary:

```sh
python3 disVM/runtime/test_managed_owner_controls.py \
    build/workspace1735/disvm-editor/disvm \
    build/workspace1735/disvm-editor/bootstrap/limbo-bootstrap \
    disVM/upstream/module
```

## Native extension usage

Include disvm/managed_reference.h and link inside the matching hosted runtime.
This header is shipped in the runtime SDK. The standalone native object-guard
library alone does NOT supply a managed heap/root registry/retirement service.

While ACTIVE, keep your original source owner and capture into a zeroed native
envelope. Accept that envelope into your native owner scope. Only after capture
succeeds may you independently drop your original owner. Export/accept or retain
native ticket owners before publishing to another OS thread; never memcpy a
filled envelope. Each receiver closes its own scope before normal return.

While outside heap admission, use only native data. A ticket keeps managed
lifetime but does not authorize reading its contents. Reenter/acknowledge progress
before borrowing through dvm_managed_owner_value; discard raw pointers before
leaving again. During native error cleanup, capture/borrow refuse and last ticket
drop merely queues Fast. Actual ptrdel/destroy run under a stopped world.

The example verifies one ticket/root with eight native receivers, 32 real Cycles
passes, shutdown refusal while owned, intact contents after reentry, a second
managed owner released through real native error cleanup, complete joins and
exact zero-pending diagnostics. It does not demonstrate parallel managed mutation,
a moving-GC pin or automatic cleanup of arbitrary Dis variables.

[Protocol, failure handling and evidence limits](https://github.com/bhgv/flashVM/blob/master/disVM/runtime/MANAGED-OWNERS.md).

The optional controls include a forced producer overlap and a producer-pin removal,
not only root removal. When using an instrumented interpreter under TSan, pass
--positive-only before the three paths to run the joined overlap independently.
Fatal negative processes deliberately stop before joining and are tested separately
normally/under UBSan; a TSan thread-leak report is never counted as control success.


For a local receiver, runtime0.75 provides `dvm_managed_owner_capture_into`.
Open your local scope, keep a zeroed entry alive through its cleanup, and pass
that pair with your independently owned managed value. On success the entry
already owns a rooted ticket; close/drop it normally. The active managed-owner
probe demonstrates this complete sequence before publishing native aliases.

Invalid or occupied destinations refuse before managed reads. A late acceptance
refusal consumes the newly captured ticket through Fast, leaving the caller's
original owner intact. Return0 can leave a queued rooted retirement; never tear
down the collectors until it drains. `dvm_owner_scope_can_accept` is a destination
check only, not a reservation across callbacks or context changes.
