"""Actual pointer-list builder: ownership and fully rolled-back failure controls."""
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
#include "kernel.h"
#include "interp.h"
#include "pool.h"
#include "pointer_builder.h"
#include "disvm/refcount.h"
#include <stdlib.h>
#include <string.h>
static int active; /* Controller-only operation interception. */
static int tested; /* Inject only once; normal retry happens after verified rollback. */
static int refused; /* One refused descriptor acquisition or allocation. */
static int skipped; /* Missing ownership operation in strict negative controls. */
void __real_dvm_runtime_pointer_cons(void *, void *);
DVM_RefcountResult __real_dvm_refcount_retain_i(int *);
DVM_RefcountResult __real_dvm_refcount_release_i(int *);
void *__real_poolalloc(Pool *, ulong);


/* Fail at exact semantic predicate before uncertain release/publication. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL pointer-builder control: %s\n", message);
        exit(10);
    }
}


/* Remove one Tptr owner or refuse acquisition, without mutating other counts. */
DVM_RefcountResult __wrap_dvm_refcount_retain_i(int *reference)
{
    if (active && CONTROL == 6 && reference == &Tlist.ref && !refused)
    {
        refused = 1;
        return DVM_REFCOUNT_REFUSED;
    }
    if (active && reference == &Tptr.ref)
    {
        if (CONTROL == 1 && !skipped)
        {
            skipped = 1;
            return DVM_REFCOUNT_LIVE;
        }
        if ((CONTROL == 2 || CONTROL == 4) && !refused)
        {
            refused = 1;
            return DVM_REFCOUNT_REFUSED;
        }
    }
    return __real_dvm_refcount_retain_i(reference);
}


/* Variant4 removes only rollback of the already acquired Tlist descriptor. */
DVM_RefcountResult __wrap_dvm_refcount_release_i(int *reference)
{
    if (active && CONTROL == 4 && reference == &Tlist.ref && !skipped)
    {
        skipped = 1;
        return DVM_REFCOUNT_LIVE;
    }
    return __real_dvm_refcount_release_i(reference);
}


/* Allocation refuses before storage creation, after owners were acquired. */
void *__wrap_poolalloc(Pool *pool, ulong bytes)
{
    if (active && CONTROL == 3 && !refused)
    {
        refused = 1;
        return nil;
    }
    return __real_poolalloc(pool, bytes);
}


/* A monitor error must roll back without invoking that monitor for destruction.
 * Borrowed node cannot escape, mutate ownership or leave heap admission. */
static void failing_monitor(int event, void *node, ulong bytes)
{
    (void)node;
    (void)bytes;
    check(event == 0, "rollback must not reenter failing monitor");
    refused = 1;
    error("injected inline builder monitor failure");
}


/* Run real builder through actual VM exception boundary; argument owns slots. */
static int invoke(void *argument)
{
    void **pair = argument;
    __real_dvm_runtime_pointer_cons(pair[0], pair[1]);
    return 1;
}


/* First builder invocation belongs to the real diagnostic. Failure variants
 * verify exact source/destination/descriptor restoration before a clean retry.
 * This is a test retry after rollback, never after partial acquisition. */
void __wrap_dvm_runtime_pointer_cons(void *source, void *destination)
{
    void *value;
    void *previous;
    void *current;
    void *pair[2];
    ulong references;
    int lists;
    int pointers;
    void (*monitor)(int, void *, ulong) = heapmonitor;
    if (tested || !CONTROL)
    {
        __real_dvm_runtime_pointer_cons(source, destination);
        return;
    }
    tested = 1;
    if (CONTROL == 1)
    {
        active = 1;
        __real_dvm_runtime_pointer_cons(source, destination);
        active = 0;
        return;
    }
    memcpy(&value, source, sizeof(value));
    memcpy(&previous, destination, sizeof(previous));
    references = heapref(value);
    lists = dvm_refcount_load_i(&Tlist.ref);
    pointers = dvm_refcount_load_i(&Tptr.ref);
    pair[0] = source;
    pair[1] = destination;
    active = 1;
    if (CONTROL == 5)
    {
        heapmonitor = failing_monitor;
    }
    check(!kprotect(invoke, pair) && refused, "injected failure reaches real VM error");
    heapmonitor = monitor;
    active = 0;
    memcpy(&current, destination, sizeof(current));
    check(current == previous && heapref(value) == references &&
        dvm_refcount_load_i(&Tlist.ref) == lists &&
        dvm_refcount_load_i(&Tptr.ref) == pointers, "transaction failure restores owners");
    print("PASS pointer-builder control: complete rollback before retry\n");
    __real_dvm_runtime_pointer_cons(source, destination);
}
'''


# Matching console/compiler/includes are required. Normal/failure positives join
# actual runtime; missing-owner/rollback controls require exact semantic10.
def main():
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:])
    with tempfile.TemporaryDirectory(prefix='pointer builder controls ') as directory:
        work = Path(directory)
        source = work / 'wrapper.c'
        source.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text('''implement S;
include "sys.m";
S: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values: list of string;
    value := "inline";
    values = value :: values;
    sys->print("builder inline %d\\n", len values);
}
''')
        subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], check=True, capture_output=True)
        for variant in range(7):
            binary = relink(runtime, work, source, variant,
                            ['dvm_runtime_pointer_cons', 'dvm_refcount_retain_i',
                             'dvm_refcount_release_i', 'poolalloc'])
            flags = ['--gc-inline'] if variant == 5 else ['--gc-worker', '--managed-owner-probe']
            result = subprocess.run([str(binary), '--no-jit', *flags, str(image)],
                                    env=dict(os.environ, TMPDIR=str(work)), capture_output=True, text=True, timeout=45)
            output = result.stdout+result.stderr
            if 'runtime error:' in output or 'WARNING: ThreadSanitizer:' in output:
                raise RuntimeError('builder sanitizer failure: '+output[-6000:])
            if variant in (1, 4):
                marker = ('FAIL pointer-builder probe: published node owns descriptors' if variant == 1 else
                          'FAIL pointer-builder control: transaction failure restores owners')
                if result.returncode != 10 or marker not in output:
                    raise RuntimeError('builder semantic control missed: '+output[-6000:])
            elif result.returncode or (variant != 5 and 'PASS pointer-builder probe:' not in output) or (
                    variant and 'PASS pointer-builder control: complete rollback before retry' not in output):
                raise RuntimeError('builder positive failed: '+output[-6000:])
    print('PASS actual pointer builder: monitor/alias/Fast, Type/allocation refusal rollback, missing-owner/rollback controls')


if __name__ == '__main__':
    main()
