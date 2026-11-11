# Copyright (c) 2026 bhgv
"""EN: Real managed-owner integer conversions with joint-guard semantic controls.
RU: Проверки целочисленных преобразований и защищённого доступа.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Convert;
include "sys.m";
Convert: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN integer conversions\\n");
    for (i := 0; i < 64; i++) {
        b := byte (i+128);
        w := int b;
        l := big w;
        back := int l;
        if (w != i+128 || back != w) {raise "integer conversion mismatch";}
    }
    sys->print("PASS integer conversions64\\n");
}
'''

WRAPPER = r'''
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_integer.h"
#include "integer_value.h"
#include "managed_owner.h"
#include "disvm/native_context.h"
#include <stdlib.h>
#include <string.h>

static int active; /* Probe runs on serialized ACTIVE controller, outside guest. */
static int transfers; /* Real integer-value calls observed while probe active. */
static size_t input_width; /* Independent expected source width. */
static size_t output_width; /* Independent expected destination width. */
static const void *input_cell; /* Borrowed exact unaligned input address. */
static void *output_cell; /* Borrowed exact unaligned output address. */
static int current_opcode; /* Actual opcode used through protected error path. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers);
void __real_dvm_runtime_integer_value(int, const void *, void *);
DVM_CellResult __real_dvm_managed_spans_begin(const void *, size_t, void *, size_t, DVM_CellAccess *);


/* Stop before any unsafe continuation; status 10 is required for controls. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL integer: %s\n", message);
        exit(10);
    }
}


/* Intercept the actual admission boundary, checking independent exact widths.
 * Controls omit a real claim or return refusal; neither is a modeled guard. */
DVM_CellResult __wrap_dvm_managed_spans_begin(const void *s, size_t n, void *d,
    size_t m, DVM_CellAccess *access)
{
    if (active)
    {
        check(s == input_cell && d == output_cell && n == input_width && m == output_width,
            "exact input/output spans");
        if (CONTROL == 1)
        {
            return DVM_CELL_ACQUIRED;
        }
        if (CONTROL == 3)
        {
            return DVM_CELL_REFUSED;
        }
    }
    return __real_dvm_managed_spans_begin(s, n, d, m, access);
}


/* Actual converter is reached only while guards exist. Output omission is an
 * independent semantic control, detected by an outside byte/canary oracle. */
void __wrap_dvm_runtime_integer_value(int op, const void *s, void *d)
{
    if (active)
    {
        check(dvm_native_context_object_count(dvm_native_context_current()) > 0,
            "conversion holds joint guards");
        transfers++;
        if (CONTROL == 2)
        {
            return;
        }
    }
    __real_dvm_runtime_integer_value(op, s, d);
}


/* Invoke the actual retained opcode, not just its helper, under real kprotect.
 * R operands are caller-owned native cells. Errors must leave no held guard. */
static int invoke(void *unused)
{
    extern void cvtbw(void);
    extern void cvtwb(void);
    extern void cvtws(void);
    extern void cvtsw(void);
    extern void cvtwl(void);
    extern void cvtlw(void);
    (void)unused;
    switch (current_opcode)
    {
    case ICVTBW:
        cvtbw();
        break;
    case ICVTWB:
        cvtwb();
        break;
    case ICVTWS:
        cvtws();
        break;
    case ICVTSW:
        cvtsw();
        break;
    case ICVTWL:
        cvtwl();
        break;
    case ICVTLW:
        cvtlw();
        break;
    default:
        check(0, "fixture opcode");
    }
    return 1;
}


/* op chooses conversion; bits supplies test data; source/result receive native
 * bytes and global width outputs. Return void; invalid fixture ops are not used.
 * Build the expected native result independently from unsigned bit patterns.
 * This fixture admits ordinary two's-complement native WORD64/SHORT16/LONG64.
 * Test all bytes, signed boundaries and deterministic large values. No heap keys. */
static void expected(int op, unsigned long long bits, unsigned char *source, unsigned char *result)
{
    BYTE b = (BYTE)bits;
    SHORT s = (SHORT)bits;
    WORD w = (WORD)bits;
    LONG l = (LONG)bits;
    switch (op)
    {
    case ICVTBW:
        input_width = sizeof(b);
        output_width = sizeof(w);
        memcpy(source, &b, sizeof(b));
        w = (WORD)(bits & 255);
        memcpy(result, &w, sizeof(w));
        break;
    case ICVTWB:
        input_width = sizeof(w);
        output_width = sizeof(b);
        memcpy(source, &w, sizeof(w));
        memcpy(result, &b, sizeof(b));
        break;
    case ICVTWS:
        input_width = sizeof(w);
        output_width = sizeof(s);
        memcpy(source, &w, sizeof(w));
        memcpy(result, &s, sizeof(s));
        break;
    case ICVTSW:
        input_width = sizeof(s);
        output_width = sizeof(w);
        memcpy(source, &s, sizeof(s));
        w = (bits & 32768) ? -(WORD)(65536 - (bits & 65535)) : (WORD)(bits & 65535);
        memcpy(result, &w, sizeof(w));
        break;
    case ICVTWL:
        input_width = sizeof(w);
        output_width = sizeof(l);
        memcpy(source, &w, sizeof(w));
        memcpy(result, &l, sizeof(l));
        break;
    case ICVTLW:
        input_width = sizeof(l);
        output_width = sizeof(w);
        memcpy(source, &l, sizeof(l));
        memcpy(result, &w, sizeof(w));
        break;
    }
}


/* Run real C opcodes over every partial overlap, checking full surrounding bytes.
 * Restore current REG/error text; no fake managed object is passed to collection. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    static const int operations[] = {ICVTBW, ICVTWB, ICVTWS, ICVTSW, ICVTWL, ICVTLW};
    REG saved;
    char message[ERRMAX];
    unsigned char bytes[64];
    unsigned char wanted[64];
    unsigned char before[64];
    unsigned char source[8];
    unsigned char result[8];
    size_t op;
    int delta;
    unsigned int sample;
    unsigned long long bits;
    const char *problem;
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(WORD) == 8 && sizeof(LONG) == 8 && sizeof(SHORT) == 2, "native fixture layout");
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(message, getup()->env->errstr, sizeof(message));
    active = 1;
    for (op = 0; op < sizeof(operations)/sizeof(operations[0]); op++)
    {
        current_opcode = operations[op];
        for (sample = 0; sample < 260; sample++)
        {
            bits = sample < 256 ? sample : sample == 256 ? ~0ULL : sample == 257 ? 32768ULL :
                sample == 258 ? 0x8000000000000000ULL : 0x123456789abcdef0ULL;
            expected(current_opcode, bits, source, result);
            for (delta = -7; delta <= 7; delta++)
            {
                memset(bytes, 0xa5, sizeof(bytes));
                memcpy(bytes+24, source, input_width);
                memcpy(wanted, bytes, sizeof(bytes));
                memcpy(wanted+24+delta, result, output_width);
                input_cell = bytes+24;
                output_cell = bytes+24+delta;
                DVM_R.s = (void *)input_cell;
                DVM_R.d = output_cell;
                if (CONTROL == 3)
                {
                    memcpy(before, bytes, sizeof(bytes));
                    check(!kprotect(invoke, NULL) && !memcmp(before, bytes, sizeof(bytes)), "guard refusal propagates unchanged");
                    check(!dvm_native_context_object_count(dvm_native_context_current()), "guard refusal unlocked");
                    active = 0;
                    memcpy(&DVM_R, &saved, sizeof(saved));
                    memcpy(getup()->env->errstr, message, sizeof(message));
                    print("PASS integer admission refusal\n");
                    return;
                }
                check(kprotect(invoke, NULL), "actual opcode completes");
                check(!memcmp(bytes, wanted, sizeof(bytes)), "exact conversion and canaries");
                check(!dvm_native_context_object_count(dvm_native_context_current()), "conversion unlocks");
            }
        }
        memset(bytes, 0xa5, sizeof(bytes));
        memcpy(wanted, bytes, sizeof(bytes));
        check(dvm_runtime_integer_convert(current_opcode, H, bytes) == exBounds &&
            dvm_runtime_integer_convert(current_opcode, bytes, H) == exBounds &&
            dvm_runtime_integer_convert(current_opcode, (void *)(uintptr)-1, bytes) == exBounds &&
            !memcmp(bytes, wanted, sizeof(bytes)), "geometry refusal before payload");
        DVM_R.s = H;
        DVM_R.d = bytes;
        check(!kprotect(invoke, NULL) && !memcmp(bytes, wanted, sizeof(bytes)), "opcode error unchanged");
    }
    problem = dvm_runtime_integer_convert(MAXDIS, H, H);
    check(problem && !strcmp(problem, "invalid integer conversion opcode") && !dvm_native_context_object_count(dvm_native_context_current()), "unknown opcode preflight");
    active = 0;
    {
        DVM_CellAccess held = {0};
        memset(bytes, 0xa5, sizeof(bytes));
        memcpy(wanted, bytes, sizeof(bytes));
        check(__real_dvm_managed_spans_begin(bytes, 1, bytes, 1, &held) == DVM_CELL_ACQUIRED,
            "held-guard fixture");
        check(!strcmp(dvm_runtime_integer_convert(ICVTBW, H, H), "integer conversion requires managed dispatch") &&
            !memcmp(bytes, wanted, sizeof(bytes)), "held guard preflight before operands");
        check(dvm_cell_access_end(&held), "held-guard fixture release");
    }
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, message, sizeof(message));
    check(transfers == 23400, "complete six-op overlap coverage");
    print("PASS integer probe: 23400 exact conversions and refusals\n");
}
'''


def scheduled_retry_wrapper():
    """Inject one pre-effect BUSY in the real integer guest dispatch."""
    source = '#include "disvm/native_process.h"\n#include "disvm/native_owner.h"\n'+WRAPPER
    source = source.replace('static int active;', """static Inst *scheduled_pc; /* PC after decoding the contended instruction. */
static int scheduled_replayed; /* The next matching attempt reached admission. */
static int active;""")
    source = source.replace('    return __real_dvm_managed_spans_begin(s, n, d, m, access);', """    int opcode = dvm_native_execution()->arithmetic_retry_opcode;

    if (CONTROL == 3 && !active && !scheduled_replayed &&
        (opcode == ICVTBW || opcode == ICVTWB || opcode == ICVTWS || opcode == ICVTSW || opcode == ICVTWL || opcode == ICVTLW))
    {
        if (!scheduled_pc)
        {
            scheduled_pc = DVM_R.PC;
            return DVM_CELL_BUSY;
        }
        check(scheduled_pc == DVM_R.PC, "same integer instruction is replayed");
        scheduled_replayed = 1;
        print("PASS scheduled integer retry\\n");
    }
    return __real_dvm_managed_spans_begin(s, n, d, m, access);""")
    value = 'void __wrap_dvm_runtime_integer_value(int op, const void *s, void *d)\n{'
    source = source.replace(value, value+r'''
    DVM_HeapOwnerStats owner;
    if (CONTROL == 0 && !active &&
        dvm_native_owner_statistics(dvm_native_owner_current(), &owner) &&
        (owner.phase == DVM_HEAP_NATIVE || owner.phase == DVM_HEAP_SHARED))
    {
        switch (op)
        {
        case ICVTWB:
            print("LEASED CVTWB\n");
            break;
        case ICVTBW:
            print("LEASED CVTBW\n");
            break;
        case ICVTWL:
            print("LEASED CVTWL\n");
            break;
        case ICVTLW:
            print("LEASED CVTLW\n");
            break;
        }
    }
''')
    return source


def main():
    """Link against matching actual runtime objects; require exact semantic controls."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded integer ') as directory:
        work = Path(directory)
        guest = work/'main.b'
        image = work/'main.dis'
        guest.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(guest)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(scheduled_retry_wrapper())
        variants = (0, 3) if '--positive-only' in sys.argv else (0, 1, 2, 3)
        for variant in variants:
            binary = relink(runtime, work, wrapper, variant,
                            ['dvm_runtime_root_registry_probe', 'dvm_runtime_integer_value', 'dvm_managed_spans_begin'])
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    args = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if variant == 0:
                        args += ['--native-spawn', '--parallel-integer']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(image)], capture_output=True, text=True, timeout=45,
                                         env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1, 2):
                        marker = 'conversion holds joint guards' if variant == 1 else 'exact conversion and canaries'
                        if run.returncode != 10 or 'FAIL integer: '+marker not in output:
                            raise RuntimeError('semantic control missed: '+output)
                    elif run.returncode or ('PASS integer admission refusal' if variant == 3 else 'PASS integer probe:') not in output or 'PASS integer conversions64' not in output:
                        raise RuntimeError('integer positive failed: '+output)
                    if variant == 0:
                        leased = {line for line in output.splitlines() if line.startswith('LEASED ')}
                        if leased != {'LEASED CVTWB', 'LEASED CVTBW', 'LEASED CVTWL', 'LEASED CVTLW'}:
                            raise RuntimeError('leased conversion coverage missed: '+output)
                    if variant == 3 and 'PASS scheduled integer retry' not in output:
                        raise RuntimeError('scheduler retry missed: '+output)
    print('PASS six actual integer C opcodes: 23400 overlaps, both polls; C/JIT guest; two omission controls unless positive-only')


if __name__ == '__main__':
    main()
