# Copyright (c) 2026 bhgv
"""EN: Actual HEADP/TAIL source lifetimes, interior guards, aliases and GC errors.
RU: Проверки указательных элементов списков и удержания владельцев.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink
from test_owner_tail import GUEST

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "raise.h"
#include "shared_list_pointer.h"
#include "pointer_builder.h"
#include "shared_pointer.h"
#include "list_hold.h"
#include "managed_owner.h"
#include "root_registry.h"
#include "gc_fields.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include "disvm/native_process.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static __thread int active; /* Dedicated current-owner fixture interval only. */
static __thread int fail_after = -1; /* Two actual list-root allocation refusals. */
static __thread int published; /* Distinguish result retirement from failed-copy cleanup. */
static __thread int callback; /* One consuming old-output callback. */
static __thread int release_error; /* Throw after temporary container consumption. */
static List *parent; /* Real pointer-cons allocation; protected by existing owner. */
static List *child; /* Real tail plus an independent fixture owner/root. */
static String *value; /* Real String plus independent fixture owner/root. */
static void *incoming; /* Expected projection, already independently live. */
static void *old; /* Genuine displaced output owner, not dereferenced after consume. */
static void *source_cell; /* Borrowed unaligned native reference cell. */
static void *target; /* Borrowed writable native output cell. */
static int selected; /* 0 HEADP, 1 TAIL. */
static DVM_GCWorkers *collectors; /* Actual stopped-world collector service. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
void __real_dvm_runtime_list_pointer_copy(const void *, void *, int);
const char *__real_dvm_runtime_movp_opcode(int, const void *, void *, int *);
static __thread int projection_opcode; /* Actual interior copy, after list hold. */
const char *__real_dvm_runtime_list_hold(int, const void *, DVM_ListHold *);
void __real_destroy(void *object);
void __real_error(char *message);
void *__real_malloc(size_t bytes);
void *__real_calloc(size_t count, size_t bytes);
DVM_CellResult __real_dvm_managed_cells_begin(const void *, void *, DVM_CellAccess *);
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *count);
DVM_RefcountResult __real_dvm_refcount_release_u(unsigned long *count);
int __real_dvm_runtime_root_batch_publish(DVM_RootBatch *, void *const *, size_t);


/* Exact semantic stop before uncertain count/collector/retirement continuation. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL shared list pointer: %s\n", message);
        exit(10);
    }
}


/* Scope the one private actual-source mutation, preserving independent probes. */
int dvm_test_list_pointer_active(void)
{
    return active;
}


/* Reject allocation attempts under object guards; refuse native root stages. */
static int allocation(void)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "allocation unlocked");
        if (fail_after == 0)
        {
            fail_after = -1;
            return 0;
        }
        if (fail_after > 0)
        {
            fail_after--;
        }
    }
    return 1;
}


/* Native allocation observation, leaving unrelated normal work unchanged. */
void *__wrap_malloc(size_t bytes)
{
    return allocation() ? __real_malloc(bytes) : NULL;
}


/* Actual reserved root blocks, not a modeled allocation counter. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    return allocation() ? __real_calloc(count, bytes) : NULL;
}


/* Missing container ownership control stops at the interior-work preflight. */
const char *__wrap_dvm_runtime_list_hold(int opcode, const void *source, DVM_ListHold *hold)
{
    if (active && CONTROL == 1)
    {
        hold->value = parent;
        return NULL;
    }
    return __real_dvm_runtime_list_hold(opcode, source, hold);
}


/* The real source container must remain rooted before touching its interior. */
const char *__wrap_dvm_runtime_movp_opcode(int opcode, const void *source, void *destination, int *replay)
{
    const char *problem;
    if (opcode != IHEADP && opcode != ITAIL)
    {
        return __real_dvm_runtime_movp_opcode(opcode, source, destination, replay);
    }
    if (active)
    {
        check(heapref(parent) == 2 && dvm_runtime_root_count(D2H(parent)) == 2,
            "source container retained and rooted");
    }
    projection_opcode = opcode;
    problem = __real_dvm_runtime_movp_opcode(opcode, source, destination, replay);
    projection_opcode = 0;
    return problem;
}


/* Only actual interior-field guards are omitted; the first container claim is
 * genuine. Retain monitoring stops before any unprotected ownership effect. */
DVM_CellResult __wrap_dvm_managed_cells_begin(const void *source, void *destination, DVM_CellAccess *access)
{
    static Inst *retry_pc[2]; /* Decoded PC at the first interior BUSY. */
    static int replayed[2]; /* Each projection reaches its interior claim again. */
    if (CONTROL == 0 && !active && projection_opcode &&
        dvm_native_execution()->arithmetic_retry_opcode == projection_opcode)
    {
        int route = projection_opcode == ITAIL;
        if (!retry_pc[route])
        {
            retry_pc[route] = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        if (!replayed[route])
        {
            check(retry_pc[route] == DVM_R.PC, "list projection retries same PC after hold cleanup");
            replayed[route] = 1;
            fprint(1, "PASS list interior replay %d\n", route);
        }
    }
    if (active && CONTROL == 2 && (source == (void *)parent->data || source == &parent->tail))
    {
        return DVM_CELL_ACQUIRED;
    }
    return __real_dvm_managed_cells_begin(source, destination, access);
}


/* Incoming pointer count must be acquired under its real source/destination
 * guards, independently of the temporary container's own earlier capture. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *count)
{
    if (active && incoming != H && count == &D2H(incoming)->ref)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0,
            "interior retain guarded");
    }
    return __real_dvm_refcount_retain_u(count);
}


/* Failed mark omission control is checked before any collector or stale use. */
DVM_RefcountResult __wrap_dvm_refcount_release_u(unsigned long *count)
{
    if (active && CONTROL == 7 && incoming != H && count == &D2H(incoming)->ref)
    {
        return DVM_REFCOUNT_LIVE;
    }
    return __real_dvm_refcount_release_u(count);
}


/* Missing source-container root must fail before payload capture/publication. */
int __wrap_dvm_runtime_root_batch_publish(DVM_RootBatch *batch, void *const *keys, size_t count)
{
    if (active && CONTROL == 6 && count == 1 && keys[0] == D2H(parent))
    {
        return 1;
    }
    return __real_dvm_runtime_root_batch_publish(batch, keys, count);
}


/* Native errors must not cross any cell/access lock. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "error unlocked");
    }
    projection_opcode = 0;
    __real_error(message);
}


/* The current callback consumes its owner before GC. Remaining container
 * lifetime must come from the temporary hold after its original owner is gone. */
void __wrap_destroy(void *object)
{
    int releasing = active && object == parent && release_error;
    if (active && projection_opcode && object == old)
    {
        published = 1;
    }
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "retirement unlocked");
        if (object == parent && CONTROL == 4)
        {
            return;
        }
        if (object == old && published)
        {
            void *observed;
            memcpy(&observed, target, sizeof(observed));
            check(observed == incoming, "publication precedes retirement");
            if (CONTROL == 5)
            {
                return;
            }
        }
    }
    __real_destroy(object);
    if (active && object == old && callback && published)
    {
        DVM_HeapOwner *owner = dvm_native_owner_current();
        void *empty = H;
        int index;
        callback = 0;
        memcpy(source_cell, &empty, sizeof(empty));
        ptrdel(D2H(parent));
        destroy(parent);
        check(heapref(parent) == 1 && dvm_runtime_root_count(D2H(parent)) == 1,
            "temporary container is sole rooted owner");
        check(dvm_native_owner_leave(owner), "leave for pointer-list Cycles");
        for (index = 0; index < 32; index++)
        {
            check(dvm_gc_workers_request(collectors, DVM_GC_CYCLES) &&
                dvm_gc_workers_wait_idle(collectors), "actual pointer-list Cycles");
        }
        check(dvm_native_owner_enter(owner) && parent->tail == child,
            "source container survives callback Cycles");
        DVM_R.s = H;
        DVM_R.d = H;
        DVM_R.m = H;
        release_error = 1;
        error("pointer-list consumed-output callback error");
    }
    if (releasing)
    {
        release_error = 0;
        error("pointer-list temporary release error");
    }
}


/* Dedicated real driver boundary; arguments stay native-local across errors. */
static int invoke(void *unused)
{
    (void)unused;
    published = 0;
    __real_dvm_runtime_list_pointer_copy(source_cell, target, selected);
    return 1;
}


/* Only compiled guest opcode calls reach this wrapper. */
void __wrap_dvm_runtime_list_pointer_copy(const void *source, void *destination, int tail)
{
    static int seen[2];
    __real_dvm_runtime_list_pointer_copy(source, destination, tail);
    if (!seen[tail]++)
    {
        print("PASS actual guarded list-pointer guest %d\n", tail);
    }
}


/* [GC:] Create actual pointer-cons nodes and independent fixture owners. The
 * original slot owner is consumed by callback, or by aliased output retirement.
 * Final actual Fast restores all real Type baselines, including payload Type. */
static void exercise(int tail, int alias)
{
    List *slot = H;
    unsigned char source[sizeof(void *) + 1];
    unsigned char destination[sizeof(void *) + 2];
    void *observed;
    int strings = dvm_refcount_load_i(&Tstring.ref);
    int lists = dvm_refcount_load_i(&Tlist.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    int incoming_count;
    int index;
    int color;
    selected = tail;
    value = c2string("pointer-list", 12);
    ptradd(D2H(value));
    dvm_runtime_pointer_cons(&value, &slot);
    child = slot;
    check(dvm_refcount_retain_u(&D2H(child)->ref) == DVM_REFCOUNT_LIVE, "independent tail owner");
    ptradd(D2H(child));
    dvm_runtime_pointer_cons(&value, &slot);
    parent = slot;
    ptradd(D2H(parent));
    source_cell = source + 1;
    memcpy(source_cell, &parent, sizeof(parent));
    incoming = tail ? (void *)child : (void *)value;
    incoming_count = heapref(incoming);
    old = alias ? (void *)parent : (void *)c2string("old output", 10);
    memset(destination, 0x5a, sizeof(destination));
    target = alias ? source_cell : destination + 1;
    if (!alias)
    {
        memcpy(target, &old, sizeof(old));
    }
    active = 1;
    for (index = 0; index < 2; index++)
    {
        fail_after = index;
        check(!kprotect(invoke, NULL) && fail_after == -1 && heapref(parent) == 1 &&
            dvm_runtime_root_count(D2H(parent)) == 1, "capture allocation rollback");
    }
    check(dvm_gc_field_load(&D2H(incoming)->color, &color) &&
        dvm_gc_field_store(&D2H(incoming)->color, -1), "inject incoming mark refusal");
    check(!kprotect(invoke, NULL), "actual interior mark failure caught");
    check(dvm_gc_field_store(&D2H(incoming)->color, color), "restore valid incoming mark");
    check(heapref(incoming) == incoming_count, "incoming failure rolls back");
    memcpy(&observed, target, sizeof(observed));
    check(observed == old && heapref(parent) == 1 && dvm_runtime_root_count(D2H(parent)) == 1,
        "temporary list owner baseline");
    if (!alias)
    {
        callback = 1;
        check(!kprotect(invoke, NULL) && !callback && !release_error &&
            strstr(getup()->env->errstr, "pointer-list consumed-output callback error"),
            "consuming callback completed");
    }
    else
    {
        check(kprotect(invoke, NULL), "aliased projection completed");
        ptrdel(D2H(parent));
    }
    memcpy(&observed, target, sizeof(observed));
    check(observed == incoming && (alias || (destination[0] == 0x5a &&
        destination[sizeof(destination) - 1] == 0x5a)), "result and boundary canaries");
    active = 0;
    destroy(observed);
    ptrdel(D2H(child));
    destroy(child);
    ptrdel(D2H(value));
    destroy(value);
    check(dvm_native_owner_leave(dvm_native_owner_current()), "leave for final pointer-list Fast");
    check(dvm_gc_workers_request(collectors, DVM_GC_FAST) && dvm_gc_workers_wait_idle(collectors),
        "actual final pointer-list Fast");
    check(dvm_native_owner_enter(dvm_native_owner_current()) &&
        dvm_refcount_load_i(&Tstring.ref) == strings && dvm_refcount_load_i(&Tlist.ref) == lists &&
        dvm_refcount_load_i(&Tptr.ref) == pointers, "pointer-list Type baselines");
}


/* [GC:] Genuine pointer/raw nodes exercise H payload/tail, empty source,
 * destination precedence, malformed selector and unmapped-head refusal. */
static void edges(void)
{
    List *slot = H;
    void *empty = H;
    void *cell;
    void *output = H;
    void *saved_source;
    void *saved_target;
    int lists = dvm_refcount_load_i(&Tlist.ref);
    int pointers = dvm_refcount_load_i(&Tptr.ref);
    dvm_runtime_pointer_cons(&empty, &slot);
    parent = slot;
    ptradd(D2H(parent));
    cell = parent;
    source_cell = &cell;
    target = &output;
    incoming = H;
    old = H;
    callback = 0;
    release_error = 0;
    active = 1;
    for (selected = 0; selected < 2; selected++)
    {
        check(kprotect(invoke, NULL) && output == H && heapref(parent) == 1,
            "H head and terminal tail are valid values");
        cell = H;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exNilref),
            "empty source refuses before projection");
        saved_target = target;
        target = H;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exHeap),
            "destination geometry precedes nil source");
        target = saved_target;
        cell = parent;
    }
    selected = 2;
    check(!kprotect(invoke, NULL) && output == H && heapref(parent) == 1,
        "invalid selector refuses before ownership");
    selected = 0;
    saved_source = source_cell;
    source_cell = H;
    check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exHeap),
        "invalid source cell geometry refuses");
    source_cell = saved_source;
    active = 0;
    ptrdel(D2H(parent));
    destroy(parent);
    slot = H;
    parent = cons(sizeof(void *), &slot);
    memcpy(parent->data, &empty, sizeof(empty));
    ptradd(D2H(parent));
    cell = parent;
    active = 1;
    check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exType) && heapref(parent) == 1,
        "raw payload cannot supply a mapped head owner");
    selected = 1;
    check(kprotect(invoke, NULL) && output == H && heapref(parent) == 1,
        "raw list still has a valid terminal tail");
    active = 0;
    ptrdel(D2H(parent));
    destroy(parent);
    check(dvm_native_owner_leave(dvm_native_owner_current()), "leave for edge Fast");
    check(dvm_gc_workers_request(collectors, DVM_GC_FAST) && dvm_gc_workers_wait_idle(collectors),
        "actual edge Fast");
    check(dvm_native_owner_enter(dvm_native_owner_current()) &&
        dvm_refcount_load_i(&Tlist.ref) == lists && dvm_refcount_load_i(&Tptr.ref) == pointers,
        "edge Type baselines");
}


/* [GC:] Borrow real collector service. No fake heap headers or module metadata;
 * source fixture uses native byte-aligned cells and actual pointer constructor. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    REG saved;
    char previous[ERRMAX];
    __real_dvm_runtime_root_registry_probe(workers);
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(previous, getup()->env->errstr, sizeof(previous));
    collectors = workers;
    exercise(0, 0);
    exercise(1, 0);
    exercise(0, 1);
    exercise(1, 1);
    edges();
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, previous, sizeof(previous));
    puts("PASS actual list pointers: HEADP/TAIL,unaligned,rollback,sole-root Cycles,source aliases,errors,Fast");
}
'''


def main():
    """Run actual C/JIT, both GC polls and safe source/boundary omission controls."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    markers = {1: 'source container retained and rooted', 2: 'interior retain guarded',
               3: 'publication precedes retirement', 4: 'temporary list owner baseline',
               5: 'consuming callback completed', 6: 'source container retained and rooted',
               7: 'incoming failure rolls back'}
    with tempfile.TemporaryDirectory(prefix='actual list pointers ') as directory:
        work = Path(directory)
        source = work / 'main.b'
        image = work / 'main.dis'
        source.write_text(GUEST)
        built = subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ([0] if '--positive-only' in sys.argv else [0, *markers]):
            private = None
            if variant == 3:
                code = (Path(__file__).resolve().parent / 'shared_pointer.c').read_text()
                before = '        memmove(destination, &incoming, sizeof(incoming));'
                if code.count(before) != 1:
                    raise RuntimeError('pointer-list publication control drift')
                code = 'extern int dvm_test_list_pointer_active(void);\n'+code.replace(before,
                    '        if (!dvm_test_list_pointer_active())\n        {\n'+before+'\n        }')
                private = work / 'shared_pointer.c'
                private.write_text(code)
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_list_pointer_copy',
                             'dvm_runtime_movp_opcode', 'dvm_runtime_list_hold', 'destroy', 'error',
                             'malloc', 'calloc', 'dvm_managed_cells_begin', 'dvm_refcount_retain_u',
                             'dvm_refcount_release_u', 'dvm_runtime_root_batch_publish'],
                            shared_pointer_source=private)
            for mode in ('--jit', '--no-jit'):
                for locked in (False, True):
                    command = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if locked:
                        command += ['--gc-locked-poll']
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True, timeout=45,
                                         env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant:
                        if run.returncode != 10 or 'FAIL shared list pointer: '+markers[variant] not in output:
                            raise RuntimeError('list pointer control missed '+str(variant)+': '+output)
                    elif run.returncode or 'PASS actual list pointers:' not in output or \
                            'PASS actual guarded list-pointer guest 0' not in output or \
                            'PASS actual guarded list-pointer guest 1' not in output or \
                            'PASS list interior replay 0' not in output or 'PASS list interior replay 1' not in output:
                        raise RuntimeError('list pointer positive failed '+str(run.returncode)+': '+output)
    print('PASS actual pointer-list C/JIT,both polls'+
          (' positives only' if '--positive-only' in sys.argv else '; seven semantic controls'))


if __name__ == '__main__':
    main()
