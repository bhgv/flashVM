"""Check actual direct root capture and late-accept Fast rollback."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "managed_owner.h"
#include "managed_owner_probe.h"
#include "disvm/native_owner.h"
#include <stdlib.h>
static DVM_GCWorkers *collectors; /* Borrowed for synchronous controller checks. */
static int intercept; /* Owner-local acceptance interception, never worker-read. */
void __real_dvm_managed_owner_probe_run(DVM_NativeWorld *, DVM_GCWorkers *);
int __real_dvm_managed_owner_capture_into(void *, DVM_OwnerScope *, DVM_OwnerEntry *);
int __real_dvm_owner_scope_accept(DVM_OwnerScope *, DVM_OwnerEntry *, DVM_OwnerTransfer *);
int __real_dvm_owner_transfer_cancel(DVM_OwnerTransfer *);


/* Retain matching collectors through the real startup diagnostic only. */
void __wrap_dvm_managed_owner_probe_run(DVM_NativeWorld *world, DVM_GCWorkers *workers)
{
    collectors = workers;
    __real_dvm_managed_owner_probe_run(world, workers);
    collectors = nil;
}


/* Intercept only completed direct capture's acceptance. Variant1 falsely
 * succeeds without linking; others refuse and exercise real rooted rollback. */
int __wrap_dvm_owner_scope_accept(DVM_OwnerScope *scope, DVM_OwnerEntry *entry,
    DVM_OwnerTransfer *transfer)
{
    if (intercept)
    {
        intercept = 0;
        return CONTROL == 1;
    }
    return __real_dvm_owner_scope_accept(scope, entry, transfer);
}


/* Variant3 removes only rollback consumption. Keep ticket/storage live until
 * exact queue predicate detects the omission, before any unsafe destruction. */
int __wrap_dvm_owner_transfer_cancel(DVM_OwnerTransfer *transfer)
{
    if (CONTROL == 3)
    {
        return 1;
    }
    return __real_dvm_owner_transfer_cancel(transfer);
}


/* Exact semantic stop; negative controls intentionally do not join shutdown.
 * No signal/compile/sanitizer/timeout is accepted as proof by the driver. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL direct-capture control: %s\n", message);
        exit(10);
    }
}


/* Inject only at real valid empty destination, preserving early-refusal probes.
 * The source has an independent caller owner; explicit extra root protects it
 * while Fast retires the refused capture. No arbitrary payload access granted. */
int __wrap_dvm_managed_owner_capture_into(void *value, DVM_OwnerScope *scope,
    DVM_OwnerEntry *entry)
{
    DVM_HeapOwner *owner;
    ulong baseline;
    int result;
    if (!CONTROL || !scope || !entry || entry->object || (uintptr)value <= 4096)
    {
        return __real_dvm_managed_owner_capture_into(value, scope, entry);
    }
    if (CONTROL == 1)
    {
        intercept = 1;
        return __real_dvm_managed_owner_capture_into(value, scope, entry);
    }
    owner = dvm_native_owner_current();
    baseline = heapref(value);
    ptradd(D2H(value));
    Setmark(D2H(value));
    intercept = 1;
    result = __real_dvm_managed_owner_capture_into(value, scope, entry);
    check(!result && !entry->object && !entry->scope, "late refusal leaves destination empty");
    check(heapref(value) == baseline+1 && dvm_managed_owner_pending() == 1,
        "late refusal queues rooted retirement");
    check(dvm_native_owner_leave(owner), "controller leaves for rollback Fast");
    check(dvm_gc_workers_request(collectors, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(collectors), "real rollback Fast drain");
    check(dvm_native_owner_enter(owner), "rollback controller reentry");
    check(heapref(value) == baseline && !dvm_managed_owner_pending() &&
        dvm_managed_owner_can_close(), "rollback preserves source and removes ticket");
    ptrdel(D2H(value));
    fprint(2, "PASS direct-capture control: real Fast rollback preserves source\n");
    exit(10);
}
'''


# Matching native console/compiler/includes are mandatory. Only variant0 joins
# the full runtime; refusal controls stop after exact predicates, outside TSan.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    cases = ((0, None), (1, 'FAIL managed owner probe: direct capture links rooted ticket'),
             (2, 'PASS direct-capture control: real Fast rollback preserves source'),
             (3, 'FAIL direct-capture control: late refusal queues rooted retirement'))
    with tempfile.TemporaryDirectory(prefix='direct root capture ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        source.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('implement S; S: module {init: fn();}; init() {}\n')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant, marker in cases:
            binary = relink(runtime, work, source, variant,
                            ['dvm_managed_owner_probe_run', 'dvm_managed_owner_capture_into',
                             'dvm_owner_scope_accept', 'dvm_owner_transfer_cancel'])
            result = subprocess.run([str(binary), '--no-jit', '--gc-worker', '--managed-owner-probe', str(image)],
                                    env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                raise RuntimeError('direct capture sanitizer failure: '+output[-6000:])
            if marker:
                if result.returncode != 10 or marker not in output:
                    raise RuntimeError('direct capture semantic control missed: '+output[-6000:])
            elif result.returncode or 'PASS managed owner probe:' not in output:
                raise RuntimeError('direct capture positive failed: '+output[-6000:])
    print('PASS actual direct capture, missing-link detection, real Fast late-refusal rollback and missing-cancel detection')


if __name__ == '__main__':
    main()
