"""Actual eight mixed-width shifts, protected error ordering and alias oracles."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Shifts;
include "sys.m";
Shifts: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN shifts\\n");
    for (i := 0; i < 32; i++) {
        n := i & 7;
        scale := 1 << n;
        b := byte 200;
        w := -73;
        l := big -9000000007;
        bl := b << n;
        br := b >> n;
        wl := w << n;
        wr := w >> n;
        ll := l << n;
        lr := l >> n;
        if (int bl != ((200*scale) & 255) || int br != 200/scale ||
            wl != -73*scale || wr != -((73+scale-1)/scale) ||
            ll != big -9000000007 * big scale || lr != -((big 9000000007+big scale-big 1)/big scale)) {raise "shift mismatch";}
    }
    sys->print("PASS shifts6\\n");
}
'''  # Six actual Limbo-emitted shifts; native LSR has a separate C/dispatch proof.

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_shift.h"
#include "shift_value.h"
#include "bytes_fast.h"
#include "managed_owner.h"
#include "disvm/native_context.h"
#include <stdlib.h>
#include <string.h>

extern void shlb(void); /* Actual native byte left shift. */
extern void shlw(void); /* Actual native word left shift. */
extern void shll(void); /* Actual native long left shift. */
extern void shrb(void); /* Actual native byte right shift. */
extern void shrw(void); /* Actual native signed word right shift. */
extern void shrl(void); /* Actual native signed long right shift. */
extern void lsrw(void); /* Actual native logical word right shift. */
extern void lsrl(void); /* Actual native logical long right shift. */
static const int operations[] = {ISHLB, ISHLW, ISHLL, ISHRB, ISHRW, ISHRL, ILSRW, ILSRL}; /* Independent expected order. */
static void (*const functions[])(void) = {shlb, shlw, shll, shrb, shrw, shrl, lsrw, lsrl}; /* Actual opcode entries. */
static int active; /* Controller-only probe; no synthetic heap roots. */
static size_t width; /* Independent expected value/output width. */
static const void *first_cell; /* Borrowed first input. */
static const void *count_cell; /* Borrowed native WORD input. */
static void *output_cell; /* Borrowed output. */
static unsigned int selected; /* One of eight actual operations. */
static unsigned long transfers; /* Observed successful publications. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_small_three_begin(const void *, size_t, const void *, size_t, void *, size_t, DVM_TripleAccess *);
int __real_dvm_runtime_shift_count(int, const void *, WORD *);
void __real_dvm_runtime_shift_value(int, const void *, WORD, void *);
void __real_error(char *);


/* valid/message are a fixture predicate/diagnostic; semantic10 stops safely. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL shift: %s\n", message);
        exit(10);
    }
}


/* Observe exact real value/count/output widths; controls omit/refuse admission. */
DVM_CellResult __wrap_dvm_managed_small_three_begin(const void *a, size_t an,
    const void *b, size_t bn, void *d, size_t dn, DVM_TripleAccess *access)
{
    if (active)
    {
        check(a == first_cell && b == count_cell && d == output_cell &&
            an == width && bn == sizeof(WORD) && dn == width, "exact mixed-width union");
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 3)
        {
            return DVM_CELL_REFUSED;
        }
    }
    return __real_dvm_managed_small_three_begin(a, an, b, bn, d, dn, access);
}


/* Every production count read must have live guards, including failure paths. */
int __wrap_dvm_runtime_shift_count(int op, const void *source, WORD *count)
{
    DVM_NativeContext *context = dvm_native_context_current();
    check(context && dvm_native_context_object_count(context), "count read under guards");
    return __real_dvm_runtime_shift_count(op, source, count);
}


/* Observe actual publication and inject a skipped store, never a modeled result. */
void __wrap_dvm_runtime_shift_value(int op, const void *a, WORD count, void *d)
{
    if (active)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()), "value under guards");
        transfers++;
        if (CONTROL == 2)
        {
            return;
        }
    }
    __real_dvm_runtime_shift_value(op, a, count, d);
}


/* Real VM errors occur only after unlock. Poison one semantic diagnostic safely. */
void __wrap_error(char *message)
{
    if (active)
    {
        check(!dvm_native_context_object_count(dvm_native_context_current()), "VM error after unlock");
        if (CONTROL == 4 && !strcmp(message, exInval))
        {
            message = exBounds;
        }
    }
    __real_error(message);
    abort();
}


/* Invoke selected actual opcode through real hosted kprotect. No payload model. */
static int invoke(void *unused)
{
    (void)unused;
    functions[selected]();
    return 1;
}


/* source/count/width are captured fixture bytes/scalar. Return expected bits.
 * Arithmetic-right oracle uses negative magnitude/rounding, not sign-fill code. */
static uvlong expected(const void *source, WORD count, size_t bytes, unsigned int op)
{
    uvlong value = 0;
    uvlong result;
    memmove(&value, source, bytes);
    if (op < 3)
    {
        result = value << count;
    }
    else if ((op == 4 || op == 5) && (value & (1ULL << 63)))
    {
        uvlong magnitude = 0-value;
        uvlong mask = (1ULL << count)-1;
        result = 0-((magnitude >> count) + ((magnitude & mask) != 0));
    }
    else
    {
        result = value >> count;
    }
    return bytes == 1 ? result & 255 : result;
}


/* Real owner admission, exact native64 oracle and stack buffers through cleanup.
 * workers is borrowed. Restore REG/error text after all cases and LSR dispatch. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    static const uvlong values[] = {0, 1, ~0ULL, 1ULL << 63, 0x123456789abcdef0ULL}; /* Boundary bit patterns. */
    WORD counts[11] = {-1, 0, 1, 7, 8, 31, 32, 63, 64};
    UWORD edge;
    REG saved;
    char error_text[ERRMAX];
    unsigned char bytes[64];
    unsigned char before[64];
    unsigned char wanted[64];
    unsigned int c;
    unsigned int v;
    int middle;
    int delta;
    unsigned long complete = 0;
    unsigned long invalid = 0;
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(WORD) == 8 && sizeof(LONG) == 8, "native fixture widths");
    edge = (UWORD)1 << 63;
    memmove(&counts[9], &edge, sizeof(edge));
    edge--;
    memmove(&counts[10], &edge, sizeof(edge));
    memmove(&saved, &DVM_R, sizeof(saved));
    memmove(error_text, getup()->env->errstr, sizeof(error_text));
    active = 1;
    for (selected = 0; selected < 8; selected++)
    {
        width = selected == 0 || selected == 3 ? 1 : 8;
        for (c = 0; c < 11; c++)
        {
            for (v = 0; v < sizeof(values)/sizeof(values[0]); v++)
            {
                for (middle = -7; middle <= 7; middle++)
                {
                    for (delta = -7; delta <= 7; delta++)
                    {
                        uvlong result = 0;
                        int valid = counts[c] >= 0 && counts[c] < (WORD)(width*8);
                        memset(bytes, 0xa5, sizeof(bytes));
                        memmove(bytes+24, &values[v], width);
                        memmove(bytes+24+middle, &counts[c], sizeof(WORD));
                        memmove(before, bytes, sizeof(bytes));
                        memmove(wanted, bytes, sizeof(bytes));
                        if (valid)
                        {
                            result = expected(bytes+24, counts[c], width, selected);
                            memmove(wanted+24+delta, &result, width);
                        }
                        first_cell = bytes+24;
                        count_cell = bytes+24+middle;
                        output_cell = bytes+24+delta;
                        DVM_R.m = (void *)first_cell;
                        DVM_R.s = (void *)count_cell;
                        DVM_R.d = output_cell;
                        if (CONTROL == 3)
                        {
                            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exHeap) &&
                                !memcmp(before, bytes, sizeof(bytes)), "refusal unchanged");
                            goto finish;
                        }
                        if (valid)
                        {
                            check(kprotect(invoke, NULL) && !memcmp(wanted, bytes, sizeof(bytes)), "exact result and canaries");
                            complete++;
                        }
                        else
                        {
                            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exInval) &&
                                !memcmp(before, bytes, sizeof(bytes)), "count error preserves cells");
                            invalid++;
                        }
                        check(!dvm_native_context_object_count(dvm_native_context_current()), "no leaked guard");
                    }
                }
            }
        }
    }
    check(complete == 54000 && invalid == 45000 && transfers == complete, "all value/count/alias cases");
    active = 0;
    for (selected = 0; selected < 8; selected++)
    {
        WORD count = -1;
        DVM_R.s = &count;
        DVM_R.m = H;
        DVM_R.d = H;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exInval), "invalid count beats outer cells");
        count = 0;
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds), "valid count rejects outer cells");
        check(dvm_runtime_shift(operations[selected], H, H, H) == exBounds, "bad count cell first");
    }
    {
        uvlong source = ~0ULL;
        uvlong output = 0;
        WORD count = 1;
        DVM_R.m = &source;
        DVM_R.s = &count;
        DVM_R.d = &output;
        check(dvm_runtime_bytes_fast(&DVM_R, ILSRW) && output == (source >> 1), "actual LSRW dispatch");
        check(dvm_runtime_bytes_fast(&DVM_R, ILSRL) && output == (source >> 1), "actual LSRL dispatch");
    }
    check(dvm_runtime_shift(MAXDIS, H, H, H) == exHeap, "unknown opcode first");
finish:
    check(!dvm_native_context_object_count(dvm_native_context_current()), "finish unlocked");
    active = 0;
    memmove(&DVM_R, &saved, sizeof(saved));
    memmove(getup()->env->errstr, error_text, sizeof(error_text));
    print("PASS shift probe:99000 cases or explicit admission refusal\n");
}
'''


def main():
    """Link matching actual native objects; require semantic controls, both polls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded shift ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0, 3) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_managed_small_three_begin',
                             'dvm_runtime_shift_count', 'dvm_runtime_shift_value', 'error'])
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    args = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=60,
                                         env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1, 2, 4):
                        marker = {1: 'count read under guards', 2: 'exact result and canaries', 4: 'count error preserves cells'}[variant]
                        if run.returncode != 10 or 'FAIL shift: '+marker not in output:
                            raise RuntimeError('shift control missed: '+output)
                    elif run.returncode or 'PASS shift probe:' not in output or 'PASS shifts6' not in output:
                        raise RuntimeError('shift positive failed: '+output)
    print('PASS eight actual shifts:99000 mixed-width/alias cases, six-op C/JIT guest, both polls')


if __name__ == '__main__':
    main()
