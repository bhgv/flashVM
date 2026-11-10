"""Actual hosted MOVP: guarded source retain/publication and unlocked retirement."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_shared_pointer import EXPECTED

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "disvm/native_process.h"
#include "raise.h"
#include "shared_pointer.h"
#include "managed_owner.h"
#include "gc_fields.h"
#include "root_registry.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <stdlib.h>
#include <string.h>

static void *displaced_key; /* Fixture root identity checked at actual unlock. */
static int displacement_observed; /* Exactly one successful publication boundary. */
int __real_dvm_cell_access_end(DVM_CellAccess *access);
static int active; /* Controller-only actual source/callback monitor. */
static String *incoming; /* Independently owned and rooted while active. */
static unsigned int copies; /* Actual helper completions during diagnostic. */
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *reference);
DVM_CellResult __real_dvm_managed_cells_begin(const void *source, void *destination, DVM_CellAccess *access);
const char *__real_dvm_runtime_movp_opcode(int, const void *, void *, int *);
void __real_destroy(void *value);
void __real_dvm_runtime_opcode_probe(DVM_GCWorkers *workers);
void __real_error(char *message);


/* Private source controls affect only the dedicated diagnostic interval. */
int dvm_test_shared_pointer_active(void)
{
    return active;
}


/* Semantic10 before count mutation, callback retirement or stale cell access. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL shared MOVP: %s\n", message);
        exit(10);
    }
}


/* Actual source's count must be acquired only inside the native cell interval.
 * Domain-counter retains are unrelated native metadata and pass unchanged. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *reference)
{
    if (active && reference == &D2H(incoming)->ref)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0,
            "source retain holds cell guards");
    }
    return __real_dvm_refcount_retain_u(reference);
}


/* Strict control removes only actual guarding. Stop at retained source check
 * before unprotected count effects; never accept the later invariant abort. */
DVM_CellResult __wrap_dvm_managed_cells_begin(const void *source, void *destination, DVM_CellAccess *access)
{
    static Inst *retry_pc; /* Post-decode MOVP PC at the first refusal. */
    static int replayed; /* Matching guest replay eventually reaches admission. */
    static unsigned int refusals; /* Delay access beyond the old eight-retry limit. */

    if (CONTROL == 0 && !active && !replayed &&
        dvm_native_execution()->arithmetic_retry_opcode == IMOVP)
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
        }
        check(retry_pc == DVM_R.PC, "MOVP replays the same instruction");
        unsigned int delay = getenv("DVM_TEST_MOVP_LONG_WAIT") ? 100001 : 32;
        if (refusals++ < delay)
        {
            return DVM_CELL_BUSY;
        }
        replayed = 1;
        fprint(1, "PASS scheduled MOVP retry: %ud refusals\n", delay);
    }

    if (active && CONTROL == 1)
    {
        return DVM_CELL_ACQUIRED;
    }
    return __real_dvm_managed_cells_begin(source, destination, access);
}


/* Observe the old owner while it is absent from the destination and before
 * the real guard is released. Ordinary root removal must leave the pin alone. */
int __wrap_dvm_cell_access_end(DVM_CellAccess *access)
{
    if (displaced_key && !displacement_observed)
    {
        check(dvm_runtime_root_count(displaced_key) == 1 &&
            !dvm_runtime_root_remove(displaced_key), "displaced owner rooted through unlock");
        displacement_observed = 1;
    }
    return __real_dvm_cell_access_end(access);
}


/* Observe actual helper result; no JIT/source-body stand-in is executed. */
const char *__wrap_dvm_runtime_movp_opcode(int opcode, const void *source, void *destination, int *replay)
{
    const char *problem = __real_dvm_runtime_movp_opcode(opcode, source, destination, replay);
    if (active)
    {
        copies++;
        check(!dvm_native_context_object_count(dvm_native_context_current()), "helper returns unlocked");
    }
    return problem;
}


const char *__real_dvm_runtime_movp_copy(const void *source, void *destination, void **previous);


/* Observe the synchronous adapter used by the direct ownership probe. */
const char *__wrap_dvm_runtime_movp_copy(const void *source, void *destination, void **previous)
{
    const char *problem = __real_dvm_runtime_movp_copy(source, destination, previous);
    if (active)
    {
        copies++;
        check(!dvm_native_context_object_count(dvm_native_context_current()), "helper returns unlocked");
    }
    return problem;
}


/* Displaced owner destruction is a callback boundary, outside ALL cell guards. */
void __wrap_destroy(void *value)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "retirement callback unlocked");
    }
    if (displaced_key && value != H && value && D2H(value) == displaced_key && displacement_observed)
    {
        check(!dvm_runtime_root_count(displaced_key), "stack root withdrawn before retirement");
    }
    __real_destroy(value);
}


/* Actual MOVP exceptions must likewise propagate only after native rollback. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "error callback unlocked");
    }
    __real_error(message);
}


/* Invoke the real adapted opcode through the host's actual kprotect boundary. */
static int invoke(void *unused)
{
    (void)unused;
    movp();
    return 1;
}


/* [GC:] Probe runs on ACTIVE controller. Keep actual source rooted over the
 * explicit IDLE role refusal, preserve all REG/error state and retire after
 * guard monitors end. Native byte buffers are real unaligned/overlapping slots. */
void __wrap_dvm_runtime_opcode_probe(DVM_GCWorkers *workers)
{
    REG saved;
    DVM_HeapOwner *owner = dvm_native_owner_current();
    unsigned char first[sizeof(void *) + 1];
    unsigned char second[sizeof(void *) + 1];
    unsigned char overlap[sizeof(void *) + 2];
    void *source;
    void *destination;
    void *previous;
    String *old;
    const char *problem;
    int color;
    char error_text[ERRMAX];
    __real_dvm_runtime_opcode_probe(workers);
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(error_text, getup()->env->errstr, sizeof(error_text));
    incoming = c2string("guarded-movp", 12);
    old = c2string("displaced", 9);
    ptradd(D2H(incoming));
    Setmark(D2H(incoming));
    source = incoming;
    destination = old;
    DVM_R.s = &source;
    DVM_R.d = &destination;
    active = 1;
    displaced_key = D2H(old);
    movp();
    check(displacement_observed, "displaced root reached actual unlock");
    displaced_key = NULL;
    old = H;
    check(destination == incoming && heapref(incoming) == 2, "destination publishes retained source");
    DVM_R.s = &destination;
    movp();
    check(destination == incoming && heapref(incoming) == 2, "self alias exact owners");
    source = H;
    DVM_R.s = &source;
    movp();
    check(destination == H && heapref(incoming) == 1, "nil replaces displaced owner");
    source = incoming;
    memmove(first + 1, &source, sizeof(source));
    memmove(second + 1, &destination, sizeof(destination));
    DVM_R.s = first + 1;
    DVM_R.d = second + 1;
    movp();
    memmove(&destination, second + 1, sizeof(destination));
    check(destination == incoming && heapref(incoming) == 2, "unaligned cell ownership");
    source = H;
    memmove(first + 1, &source, sizeof(source));
    movp();
    check(heapref(incoming) == 1, "unaligned clear releases owner");
    memset(overlap, 255, sizeof(overlap));
    DVM_R.s = overlap + 1;
    DVM_R.d = overlap + 2;
    movp();
    memmove(&destination, overlap + 2, sizeof(destination));
    check(destination == H, "partial overlap nil cells");
    source = incoming;
    destination = H;
    previous = (void *)55;
    color = dvm_runtime_gc_load(&D2H(incoming)->color);
    check(dvm_gc_field_store(&D2H(incoming)->color, -1), "private invalid color setup");
    problem = dvm_runtime_movp_copy(&source, &destination, &previous);
    check(problem && !strcmp(problem, exHeap) && destination == H && previous == (void *)55,
        "mark refusal preserves cells and output");
    check(heapref(incoming) == 1, "mark refusal restores retained count");
    check(dvm_gc_field_store(&D2H(incoming)->color, color), "restore live color before collection");
    source = nil;
    DVM_R.s = &source;
    DVM_R.d = &destination;
    check(!kprotect(invoke, NULL) && destination == H && heapref(incoming) == 1,
        "actual nil source error preserves owner");
    source = incoming;
    check(dvm_runtime_movp_copy(&source, &destination, &source) != NULL &&
        source == incoming && destination == H, "output alias refuses unchanged");
    check(dvm_native_owner_leave(owner), "leave for refusal preflight");
    previous = (void *)55;
    check(dvm_runtime_movp_copy(&source, &destination, &previous) != NULL && previous == (void *)55,
        "IDLE refuses before managed cells");
    check(dvm_native_owner_enter(owner), "reenter after role refusal");
    check(copies >= 9, "actual guarded copy coverage");
    active = 0;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, error_text, sizeof(error_text));
    ptrdel(D2H(incoming));
    destroy(incoming);
    incoming = H;
    check(dvm_native_owner_leave(owner) && dvm_gc_workers_request(workers, DVM_GC_FAST) &&
        dvm_gc_workers_wait_idle(workers) && dvm_native_owner_enter(owner), "actual final Fast");
    fprint(1, "PASS actual shared MOVP: aligned/unaligned/self/overlap, rollback, role/error/unlocked callbacks\n");
}
'''


def old_body(runtime):
    """Reconstruct reviewed pre-guard C body only from the pinned retained opcode.

    Exact hash must agree; this is a negative source control, not another VM.
    """
    pinned = Path(__file__).resolve().parents[1] / 'upstream/libinterp/xec.c'
    body = re.search(r'OP\(movp\)\n\{.*?\n\}', pinned.read_text(), re.S).group()
    body = body.replace('R.', 'DVM_R.').replace('h->ref++;', 'dvm_runtime_heap_retain(&h->ref);')
    body = body.replace('h->ref ==', 'dvm_refcount_load_u(&h->ref) ==')
    body = body.replace('D2H(dv)->ref ==', 'dvm_refcount_load_u(&D2H(dv)->ref) ==')
    if hashlib.sha256(body.encode()).hexdigest() != EXPECTED:
        raise RuntimeError('old MOVP source control hash drift')
    source = (runtime.parent / 'native/xec.c').read_text()
    return re.sub(r'OP\(movp\)\n\{.*?\n\}', lambda match: body, source, count=1, flags=re.S)


def main():
    """Relink actual runtime privately; C/JIT/both-poll cases and semantic stops.

    No compiler/signal/timeout/sanitizer outcome substitutes for a strict control.
    Production objects and retained snapshots remain unchanged.
    """
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    source = (Path(__file__).resolve().parent / 'shared_pointer.c').read_text()
    omitted = source.replace('        memmove(destination, &incoming, sizeof(incoming));',
                             '        if (!dvm_test_shared_pointer_active()) { memmove(destination, &incoming, sizeof(incoming)); }')
    early = source.replace('    if (selected == 2 && !dvm_cell_access_end(&access))',
                           '    if (!problem && dvm_test_shared_pointer_active()) { destroy(outgoing); }\n    if (selected == 2 && !dvm_cell_access_end(&access))')
    rollback = source.replace('if (dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)',
                              'if (!dvm_test_shared_pointer_active() && dvm_refcount_release_u(&heap->ref) != DVM_REFCOUNT_LIVE)')
    if omitted == source or early == source or rollback == source:
        raise RuntimeError('shared MOVP source control drift')
    with tempfile.TemporaryDirectory(prefix='shared pointer cells ') as directory:
        work = Path(directory)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text(r'''implement Probe;
include "sys.m";
Probe: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    values := array[] of {"first", "second"};
    copied := values[0];
    values[0] = "replacement";
    if (copied != "first")
        raise "MOVP replay changed the source";
    sys->print("PASS normal Dis\n");
}
''')
        built = subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)],
                               capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        variants = [(0, source, None, '')]
        if '--positive-only' not in sys.argv:
            variants += [(1, source, None, 'source retain holds cell guards'),
                         (2, omitted, None, 'destination publishes retained source'),
                         (3, early, None, 'retirement callback unlocked'),
                         (4, rollback, None, 'mark refusal restores retained count'),
                         (5, source, old_body(runtime), 'source retain holds cell guards')]
        for variant, code, executor, marker in variants:
            private = work / 'shared_pointer.c'
            private.write_text('extern int dvm_test_shared_pointer_active(void);\n'+code)
            if executor is not None:
                private_executor = work / 'xec.c'
                private_executor.write_text(executor)
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_refcount_retain_u', 'dvm_managed_cells_begin', 'dvm_runtime_movp_opcode', 'dvm_runtime_movp_copy',
                             'dvm_cell_access_end',
                             'destroy', 'error', 'dvm_runtime_opcode_probe'],
                            executor=private_executor if executor is not None else None,
                            shared_pointer_source=private)
            for mode in ('--jit', '--no-jit'):
                profiles = [(False, False), (True, False)]
                if variant == 0:
                    profiles.append((False, True))
                for locked, dedicated in profiles:
                    command = [str(binary), mode, '--gc-worker', '--managed-owner-probe']
                    if dedicated:
                        command.append('--native-spawn')
                    if locked:
                        command += ['--gc-locked-poll']
                    run = subprocess.run(command+[str(image)], capture_output=True, text=True,
                                         timeout=45, env=dict(os.environ, TMPDIR=str(work),
                                             **({"DVM_TEST_MOVP_LONG_WAIT": "1"} if dedicated else {})))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant:
                        if run.returncode != 10 or 'FAIL shared MOVP: '+marker not in output:
                            raise RuntimeError('shared MOVP control missed '+str(variant)+': '+output)
                    elif run.returncode or 'PASS actual shared MOVP:' not in output or 'PASS normal Dis' not in output or \
                            'PASS scheduled MOVP retry' not in output or \
                            (dedicated and 'PASS scheduled MOVP retry: 100001 refusals' not in output):
                        raise RuntimeError('shared MOVP positive failed: '+output)
    print('PASS actual shared MOVP: C/JIT, both polls, rooted real owners'+
          ('; positives only' if '--positive-only' in sys.argv else '; five semantic controls'))


if __name__ == '__main__':
    main()
