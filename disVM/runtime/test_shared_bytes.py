"""Actual scalar/raw copies and SELF: exact bits, guards and unlocked callbacks."""
import hashlib
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from relink_private import relink
from prepare_shared_bytes import EXPECTED

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "interp.h"
#include "isa.h"
#include "disvm/native_process.h"
#include "raise.h"
#include "managed_owner.h"
#include "runqueue.h"
#include "shared_bytes.h"
#include "byte_transfer.h"
#include "disvm/refcount.h"
#include "disvm/native_owner.h"
#include <fenv.h>
#include <stdlib.h>
#include <string.h>

static int active; /* Controller-only dedicated diagnostic interval. */
static ulong *self_reference; /* Independently live current module's count. */
static int transfers; /* Observed actual process-live transfer calls. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
void __real_dvm_runtime_byte_transfer(void *destination, const void *source, size_t size);
DVM_CellResult __real_dvm_managed_spans_begin(const void *, size_t, void *, size_t, DVM_CellAccess *);
DVM_CellResult __real_dvm_managed_ranges_begin(const void *, size_t, void *, size_t, DVM_RangeAccess *);
DVM_RefcountResult __real_dvm_refcount_retain_u(unsigned long *reference);
void __real_error(char *message);
void __real_destroy(void *value);
extern void movb(void);
extern void movw(void);
extern void movl(void);
extern void movf(void);
extern void movm(void);
extern void self(void);


/* Private mutation interval, not a production hook/permission boundary. */
int dvm_test_shared_bytes_active(void)
{
    return active;
}


/* Semantic10 before payload or ownership effects in strict controls. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL shared bytes: %s\n", message);
        exit(10);
    }
}


/* Independent byte-by-byte oracle for the exact native stripe union. */
static unsigned int expected(const void *source, const void *destination, size_t size)
{
    unsigned char selected[256] = {0};
    size_t index;
    unsigned int count = 0;
    for (index = 0; index < size; index++)
    {
        selected[((uintptr)source + index) / sizeof(void *) % 256] = 1;
        selected[((uintptr)destination + index) / sizeof(void *) % 256] = 1;
    }
    for (index = 0; index < 256; index++)
    {
        count += selected[index];
    }
    return count;
}


/* Actual data transfer is undefined-reference intercepted, not compiler memmove
 * substitution. Every nonempty profile copy must own its COMPLETE stripe union. */
void __wrap_dvm_runtime_byte_transfer(void *destination, const void *source, size_t size)
{
    if (active)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) == expected(source, destination, size),
            "transfer holds complete range guards");
        transfers++;
    }
    __real_dvm_runtime_byte_transfer(destination, source, size);
}


/* Inject one BUSY per copy stage and observe the matching replay. */
static int contend_copy(int stage, int opcode)
{
    static Inst *pcs[3]; /* Decoded PC at each injected refusal. */
    static int replayed[3]; /* One matching replay per stage. */

    if (CONTROL != 0 || active || replayed[stage] ||
        dvm_native_execution()->arithmetic_retry_opcode != opcode)
    {
        return 0;
    }
    if (!pcs[stage])
    {
        pcs[stage] = DVM_R.PC;
        return 1;
    }
    check(pcs[stage] == DVM_R.PC, "copy replay uses the same PC");
    replayed[stage] = 1;
    fprint(1, "PASS scheduled copy stage %d\n", stage);
    return 0;
}


/* Two controls remove only actual native acquisition; monitors stop BEFORE copy. */
DVM_CellResult __wrap_dvm_managed_spans_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_CellAccess *access)
{
    if (contend_copy(0, IMOVW))
    {
        return DVM_CELL_BUSY;
    }

    if (active && CONTROL == 1)
    {
        return DVM_CELL_ACQUIRED;
    }
    return __real_dvm_managed_spans_begin(source, source_size, destination, destination_size, access);
}


/* Wide storage uses the exact same domain as scalar and pointer copies. */
DVM_CellResult __wrap_dvm_managed_ranges_begin(const void *source, size_t source_size,
    void *destination, size_t destination_size, DVM_RangeAccess *access)
{
    if (contend_copy(destination ? 2 : 1, IMOVM))
    {
        return DVM_CELL_BUSY;
    }

    if (active && !destination && !destination_size)
    {
        DVM_CellResult result = __real_dvm_managed_ranges_begin(source, source_size,
            destination, destination_size, access);
        check(source == DVM_R.m && source_size == sizeof(WORD) &&
            result == DVM_CELL_ACQUIRED &&
            dvm_native_context_object_count(dvm_native_context_current()) > 0,
            "MOVM count snapshot holds source guard");
        return result;
    }
    if (active && CONTROL == 2)
    {
        return DVM_CELL_ACQUIRED;
    }
    return __real_dvm_managed_ranges_begin(source, source_size, destination, destination_size, access);
}


/* SELF must retain its current module only inside MOVP's cell interval. */
DVM_RefcountResult __wrap_dvm_refcount_retain_u(unsigned long *reference)
{
    if (active && reference == self_reference)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0, "SELF retain guarded");
    }
    return __real_dvm_refcount_retain_u(reference);
}


/* Error and retirement are actual callback boundaries, always outside guards. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "error callback unlocked");
    }
    __real_error(message);
}


/* No old module destruction occurs under the copy interval. */
void __wrap_destroy(void *value)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "retirement callback unlocked");
    }
    __real_destroy(value);
}


/* Call actual MOVM through native exception protection. */
static int bad_copy(void *unused)
{
    (void)unused;
    movm();
    return 1;
}


/* [GC:] Preserve REG/FP/error state around actual opcode calls. buffers are
 * native mapped bytes, not fabricated managed Heaps or roots. workers borrowed;
 * no access survives the original real-root Cycles/Fast diagnostic afterwards. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    REG saved;
    Prog *queued;
    unsigned char data[8192];
    unsigned char oracle[8192];
    unsigned char destination[8192];
    char error_text[ERRMAX];
    fenv_t floating;
    WORD count;
    void *module_result = H;
    void *nil_value = H;
    unsigned long references;
    size_t sizes[] = {1, sizeof(WORD), sizeof(LONG), sizeof(REAL), 17, 6000};
    void (*operations[])(void) = {movb, movw, movl, movf, movm, movm};
    unsigned int index;
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(error_text, getup()->env->errstr, sizeof(error_text));
    fegetenv(&floating);
    for (index = 0; index < sizeof(data); index++)
    {
        data[index] = (unsigned char)(index * 37 + 11);
    }
    active = 1;
    for (index = 0; index < 6; index++)
    {
        size_t size = sizes[index];
        memset(destination, 0xa5, sizeof(destination));
        count = (WORD)size;
        DVM_R.s = data + 1;
        DVM_R.d = destination + 3;
        DVM_R.m = &count;
        operations[index]();
        check(!memcmp(data + 1, destination + 3, size) && destination[2] == 0xa5 &&
            destination[3 + size] == 0xa5, "exact scalar and wide payload");
    }
    memcpy(oracle, data, sizeof(data));
    memmove(oracle + 5, oracle + 1, 6000);
    count = 6000;
    DVM_R.s = data + 1;
    DVM_R.d = data + 5;
    DVM_R.m = &count;
    movm();
    check(!memcmp(data, oracle, sizeof(data)), "wide overlap matches memmove");
    {
        uvlong bits = 0xfff0000000000001ULL;
        uvlong result;
        int exceptions;
        memcpy(data + 1, &bits, sizeof(bits));
        DVM_R.s = data + 1;
        DVM_R.d = destination + 3;
        feraiseexcept(FE_INEXACT);
        exceptions = fetestexcept(FE_ALL_EXCEPT);
        movf();
        memcpy(&result, destination + 3, sizeof(result));
        check(result == bits && fetestexcept(FE_ALL_EXCEPT) == exceptions, "REAL preserves NaN bits and FP flags");
    }
    count = 0;
    DVM_R.m = &count;
    DVM_R.s = nil;
    DVM_R.d = nil;
    movm();
    count = -1;
    check(!kprotect(bad_copy, NULL), "negative MOVM raises outside guards");
    check(dvm_runtime_bytes_copy(NULL, NULL, 0) == NULL &&
        dvm_runtime_bytes_copy((void *)(uintptr)-2, destination, 4) != NULL,
        "zero and wrapped primitive spans");
    /* Startup diagnostics run before a Prog is claimed: native REG.M is nil.
     * Borrow the genuine queued Prog's rooted module under ACTIVE exclusion,
     * without changing process ownership or executing that Prog. */
    queued = dvm_runqueue_head();
    check(queued && queued->R.M != nil && queued->R.M != H, "real startup module source");
    DVM_R.M = queued->R.M;
    self_reference = &D2H(DVM_R.M)->ref;
    references = dvm_refcount_load_u(self_reference);
    DVM_R.d = &module_result;
    DVM_R.s = (void *)1;
    self();
    check(module_result == DVM_R.M && dvm_refcount_load_u(self_reference) == references + 1,
        "SELF publishes current module");
    self();
    check(dvm_refcount_load_u(self_reference) == references + 1, "SELF alias exact owners");
    DVM_R.s = &nil_value;
    movp();
    check(module_result == H && dvm_refcount_load_u(self_reference) == references, "SELF previous owner retired");
    self_reference = NULL;
    check(transfers >= 8 && !dvm_native_context_object_count(dvm_native_context_current()), "actual guarded transfer coverage");
    active = 0;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, error_text, sizeof(error_text));
    fesetenv(&floating);
    __real_dvm_runtime_root_registry_probe(workers);
    fprint(1, "PASS shared byte/SELF probe: exact bits/overlap/FP/empty/error, guarded publication and unlocked callbacks\n");
}
'''


def main():
    """Real C/JIT/both-poll probes; strict scoped source controls stop semantic10."""
    runtime, compiler, include = (Path(value).resolve() for value in sys.argv[1:4])
    source = (Path(__file__).resolve().parent / 'shared_bytes.c').read_text()
    transfer = '    dvm_runtime_byte_transfer(destination, source, size);'
    if source.count(transfer) != 3:
        raise RuntimeError('shared byte transfer control drift')
    omitted = source.replace(transfer, '    if (!dvm_test_shared_bytes_active()) { dvm_runtime_byte_transfer(destination, source, size); }')
    xec = (runtime.parent / 'native/xec.c').read_text()
    # Restore only old SELF from the pinned retained source and reviewed count view.
    pinned = Path(__file__).resolve().parents[1] / 'upstream/libinterp/xec.c'
    old = re.search(r'OP\(self\)\n\{.*?\n\}', pinned.read_text(), re.S).group()
    old = old.replace('R.', 'DVM_R.').replace('h->ref++;', 'dvm_runtime_heap_retain(&h->ref);')
    old = old.replace('h->ref ==', 'dvm_refcount_load_u(&h->ref) ==')
    old = old.replace('D2H(t)->ref ==', 'dvm_refcount_load_u(&D2H(t)->ref) ==')
    if hashlib.sha256(old.encode()).hexdigest() != EXPECTED['self']:
        raise RuntimeError('old SELF source control hash drift')
    old_self = re.sub(r'OP\(self\)\n\{.*?\n\}', lambda match: old, xec, count=1, flags=re.S)
    with tempfile.TemporaryDirectory(prefix='shared byte copies ') as directory:
        work = Path(directory)
        wrapper = work / 'wrapper.c'
        wrapper.write_text(WRAPPER)
        guest = work / 'main.b'
        image = work / 'main.dis'
        guest.write_text(r'''implement Probe;
include "sys.m";
Probe: module {init: fn();};
Triple: adt { first, second, third: int; };
init() {
    sys := load Sys Sys->PATH;
    values := array[] of {3, 5, 7};
    item := Triple(values[0], values[1], values[2]);
    copy := item;
    if (copy.first != 3 || copy.second != 5 || copy.third != 7)
        raise "copy replay changed payload";
    sys->print("PASS normal Dis\n");
}
''')
        built = subprocess.run([str(compiler), '-I', str(include), '-o', str(image), str(guest)], capture_output=True, text=True)
        if built.returncode:
            raise RuntimeError(built.stdout+built.stderr)
        variants = [(0, source, None, '')]
        if '--positive-only' not in sys.argv:
            variants += [(1, source, None, 'transfer holds complete range guards'),
                         (2, source, None, 'transfer holds complete range guards'),
                         (3, omitted, None, 'exact scalar and wide payload'),
                         (4, source, old_self, 'SELF retain guarded')]
        for variant, code, executor, marker in variants:
            private = work / 'shared_bytes.c'
            private.write_text('extern int dvm_test_shared_bytes_active(void);\n'+code)
            private_executor = None
            if executor:
                private_executor = work / 'xec.c'
                private_executor.write_text(executor)
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_byte_transfer',
                             'dvm_managed_spans_begin', 'dvm_managed_ranges_begin', 'dvm_refcount_retain_u', 'error', 'destroy'],
                            shared_bytes_source=private, executor=private_executor)
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
                        if run.returncode != 10 or 'FAIL shared bytes: '+marker not in output:
                            raise RuntimeError('shared byte control missed '+str(variant)+': '+output)
                    elif run.returncode or 'PASS shared byte/SELF probe:' not in output or 'PASS normal Dis' not in output:
                        raise RuntimeError('status='+str(run.returncode)+' mode='+mode+' locked='+str(locked)+'\n'+output)
                    if variant == 0 and any(f'PASS scheduled copy stage {stage}' not in output
                                            for stage in range(3)):
                        raise RuntimeError('copy replay coverage missed: '+output)
        refused = subprocess.run([str(runtime), '--guarded-copies', '--gc-inline', str(image)], capture_output=True, text=True)
        if refused.returncode != 2 or '--guarded-copies requires --gc-worker' not in refused.stderr:
            raise RuntimeError('missing explicit inline-profile refusal')
    print('PASS actual guarded bytes and SELF: C/JIT, both polls'+
          ('; positives only' if '--positive-only' in sys.argv else '; four semantic controls'))


if __name__ == '__main__':
    main()
