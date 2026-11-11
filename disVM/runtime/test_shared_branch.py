# Copyright (c) 2026 bhgv
"""EN: Actual eighteen guarded branches: aliases, lazy targets and semantic controls.
RU: Проверки ветвлений, блокировок операндов и неизменности при отказах.
"""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
from relink_private import relink

GUEST = '''implement Branches;
include "sys.m";
Branches: module {init: fn();};
init() {
    sys := load Sys Sys->PATH;
    sys->print("BEGIN branches\\n");
    score := 0;
    for (i := 0; i < 8; i++) {
        b := array[2] of byte;
        w := array[2] of int;
        l := array[2] of big;
        b[0] = byte (i & 31);
        b[1] = byte ((i & 31)+1);
        w[0] = -(i & 31)-1;
        w[1] = i & 31;
        l[0] = big -9000000007-big (i & 31);
        l[1] = big 9000000007;
        if (b[0] == b[1]) {
            score++;
        }
        if (b[0] != b[1]) {
            score++;
        }
        if (b[0] < b[1]) {
            score++;
        }
        if (b[0] <= b[1]) {
            score++;
        }
        if (b[0] > b[1]) {
            score++;
        }
        if (b[0] >= b[1]) {
            score++;
        }
        if (w[0] == w[1]) {
            score++;
        }
        if (w[0] != w[1]) {
            score++;
        }
        if (w[0] < w[1]) {
            score++;
        }
        if (w[0] <= w[1]) {
            score++;
        }
        if (w[0] > w[1]) {
            score++;
        }
        if (w[0] >= w[1]) {
            score++;
        }
        if (l[0] == l[1]) {
            score++;
        }
        if (l[0] != l[1]) {
            score++;
        }
        if (l[0] < l[1]) {
            score++;
        }
        if (l[0] <= l[1]) {
            score++;
        }
        if (l[0] > l[1]) {
            score++;
        }
        if (l[0] >= l[1]) {
            score++;
        }
    }
    if (score != 72) {
        raise "branch mismatch";
    }
    sys->print("PASS branches18\\n");
}
'''  # Eighteen actual typed comparisons, nine true per iteration.
WRAPPER = r'''

#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "raise.h"
#include "shared_branch.h"
#include "branch_value.h"
#include "bytes_fast.h"
#include "managed_owner.h"
#include "disvm/native_context.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include <stdlib.h>
#include <string.h>

extern void beqb(void); /* Actual integer conditional branch. */
extern void bneb(void); /* Actual integer conditional branch. */
extern void bltb(void); /* Actual integer conditional branch. */
extern void bleb(void); /* Actual integer conditional branch. */
extern void bgtb(void); /* Actual integer conditional branch. */
extern void bgeb(void); /* Actual integer conditional branch. */
extern void beqw(void); /* Actual integer conditional branch. */
extern void bnew(void); /* Actual integer conditional branch. */
extern void bltw(void); /* Actual integer conditional branch. */
extern void blew(void); /* Actual integer conditional branch. */
extern void bgtw(void); /* Actual integer conditional branch. */
extern void bgew(void); /* Actual integer conditional branch. */
extern void beql(void); /* Actual integer conditional branch. */
extern void bnel(void); /* Actual integer conditional branch. */
extern void bltl(void); /* Actual integer conditional branch. */
extern void blel(void); /* Actual integer conditional branch. */
extern void bgtl(void); /* Actual integer conditional branch. */
extern void bgel(void); /* Actual integer conditional branch. */
static const int operations[] = {IBEQB, IBNEB, IBLTB, IBLEB, IBGTB, IBGEB, IBEQW, IBNEW, IBLTW, IBLEW, IBGTW, IBGEW, IBEQL, IBNEL, IBLTL, IBLEL, IBGTL, IBGEL}; /* Existing instruction order. */
static void (*const functions[])(void) = {beqb, bneb, bltb, bleb, bgtb, bgeb, beqw, bnew, bltw, blew, bgtw, bgew, beql, bnel, bltl, blel, bgtl, bgel}; /* Actual C entries. */
static unsigned int selected; /* Fixture owner-local operation index. */
static int active; /* Width/publication probe is scoped to ordinary cases. */
static size_t width; /* Independent BYTE/WORD/LONG source extent. */
static const void *first_cell; /* Borrowed complete input slot. */
static const void *other_cell; /* Borrowed complete other slot. */
static void *target_cell; /* Borrowed complete pointer slot. */
static unsigned long predicates; /* Observed actual protected comparisons. */
static unsigned long publications; /* Observed actual taken-target calls. */
void __real_dvm_runtime_root_registry_probe(DVM_GCWorkers *);
DVM_CellResult __real_dvm_managed_small_three_begin(const void *, size_t, const void *, size_t, void *, size_t, DVM_TripleAccess *);
int __real_dvm_runtime_branch_predicate(int, const void *, const void *);
int __real_dvm_runtime_branch_target(const void *, void *);
void __real_error(char *);


/* valid/message describe an independent fixture predicate. Exit semantic10 on
 * failure; no compile/signal/sanitizer failure can satisfy an omission control. */
static void check(int valid, const char *message)
{
    if (!valid)
    {
        fprint(2, "FAIL branch: %s\n", message);
        exit(10);
    }
}


/* Observe exact normal source/other/target union; controls omit/refuse admission. */
DVM_CellResult __wrap_dvm_managed_small_three_begin(const void *a, size_t an,
    const void *b, size_t bn, void *d, size_t dn, DVM_TripleAccess *access)
{
    static Inst *retry_pc; /* Decoded PC of the guest branch selected for replay. */
    static unsigned long retry_count; /* Instruction quota position before BUSY. */
    static int replayed; /* The selected branch has reached admission again. */
    int opcode = dvm_native_execution()->arithmetic_retry_opcode;

    if (CONTROL == 3 && !active && !replayed &&
        (opcode == IBEQB || opcode == IBNEB || opcode == IBLTB || opcode == IBLEB ||
         opcode == IBGTB || opcode == IBGEB || opcode == IBEQW || opcode == IBNEW ||
         opcode == IBLTW || opcode == IBLEW || opcode == IBGTW || opcode == IBGEW ||
         opcode == IBEQL || opcode == IBNEL || opcode == IBLTL || opcode == IBLEL ||
         opcode == IBGTL || opcode == IBGEL))
    {
        if (!retry_pc)
        {
            retry_pc = DVM_R.PC;
            retry_count = dvm_runtime_instructions;
            return DVM_CELL_BUSY;
        }
        check(DVM_R.PC == retry_pc && dvm_runtime_instructions == retry_count,
            "same branch replays without charging twice");
        replayed = 1;
        print("PASS scheduled branch retry\n");
    }
    if (active)
    {
        check(a == first_cell && b == other_cell && d == target_cell &&
            an == width && bn == width && dn == sizeof(Inst *), "exact branch union");
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


/* Every source snapshot occurs under actual guards, even the false condition. */
int __wrap_dvm_runtime_branch_predicate(int op, const void *a, const void *b)
{
    int result;
    DVM_HeapOwnerStats owner;
    if (CONTROL == 0 && !active &&
        dvm_native_owner_statistics(dvm_native_owner_current(), &owner) &&
        (owner.phase == DVM_HEAP_NATIVE || owner.phase == DVM_HEAP_SHARED))
    {
        check((dvm_native_context_object_count(dvm_native_context_current()) != 0) ==
            (owner.phase == DVM_HEAP_SHARED), "predicate guard matches its owner phase");
        fprint(2, "LEASED branch %d\n", op);
        return __real_dvm_runtime_branch_predicate(op, a, b);
    }
    check(dvm_native_context_object_count(dvm_native_context_current()), "predicate under guards");
    result = __real_dvm_runtime_branch_predicate(op, a, b);
    if (active)
    {
        predicates++;
        if (CONTROL == 4)
        {
            return !result;
        }
    }
    return result;
}


/* A taken target is read/published while guards are live. Control skips only
 * real publication; false predicates must never invoke this function. */
int __wrap_dvm_runtime_branch_target(const void *target, void *pc)
{
    check(dvm_native_context_object_count(dvm_native_context_current()), "target under guards");
    if (active)
    {
        publications++;
        if (CONTROL == 2)
        {
            return 1;
        }
    }
    return __real_dvm_runtime_branch_target(target, pc);
}


/* Real VM error entry must follow complete guard release, including bad targets. */
void __wrap_error(char *message)
{
    check(!dvm_native_context_object_count(dvm_native_context_current()), "VM error after unlock");
    __real_error(message);
    abort();
}


/* Invoke the selected actual C opcode through its owner's real error frame. */
static int invoke(void *unused)
{
    (void)unused;
    functions[selected]();
    return 1;
}


/* Borrow captured byte spans; derive a three-way ordering then condition truth.
 * This oracle does not call production predicates or access mutable REG slots. */
static int expected(const void *a, const void *b, unsigned int operation)
{
    int order;
    if (operation < 6)
    {
        unsigned char x;
        unsigned char y;
        memcpy(&x, a, sizeof(x));
        memcpy(&y, b, sizeof(y));
        order = (x > y)-(x < y);
    }
    else
    {
        long long x;
        long long y;
        memcpy(&x, a, sizeof(x));
        memcpy(&y, b, sizeof(y));
        order = (x > y)-(x < y);
    }
    switch (operation % 6)
    {
    case 0: return order == 0;
    case 1: return order != 0;
    case 2: return order == -1;
    case 3: return order != 1;
    case 4: return order == 1;
    default: return order != -1;
    }
}


/* workers is borrowed by the actual root probe. Own all fixture cells until REG
 * restoration; no fake Heap headers/roots or cross-thread code pointers. */
void __wrap_dvm_runtime_root_registry_probe(DVM_GCWorkers *workers)
{
    static const uvlong values[] = {0, 1, ~0ULL, 1ULL << 63, 0x123456789abcdef0ULL}; /* Boundary bits. */
    REG saved;
    char error_text[ERRMAX];
    Inst code[2];
    unsigned char cells[64];
    unsigned char before[64];
    unsigned long cases = 0;
    unsigned long taken = 0;
    unsigned int a;
    unsigned int b;
    int other_offset;
    int target_offset;
    __real_dvm_runtime_root_registry_probe(workers);
    check(sizeof(WORD) == 8 && sizeof(LONG) == 8 && sizeof(Inst *) == 8, "native fixture widths");
    memcpy(&saved, &DVM_R, sizeof(saved));
    memcpy(error_text, getup()->env->errstr, sizeof(error_text));
    active = 1;
    for (selected = 0; selected < 18; selected++)
    {
        width = selected < 6 ? 1 : 8;
        for (a = 0; a < 5; a++)
        {
            for (b = 0; b < 5; b++)
            {
                for (other_offset = -7; other_offset <= 7; other_offset++)
                {
                    for (target_offset = -7; target_offset <= 7; target_offset++)
                    {
                        Inst *destination = &code[1];
                        int truth;
                        memset(cells, 0xa5, sizeof(cells));
                        memcpy(cells+24, &values[a], width);
                        memcpy(cells+24+other_offset, &values[b], width);
                        memcpy(cells+24+target_offset, &destination, sizeof(destination));
                        memcpy(before, cells, sizeof(cells));
                        first_cell = cells+24;
                        other_cell = cells+24+other_offset;
                        target_cell = cells+24+target_offset;
                        truth = expected(first_cell, other_cell, selected);
                        DVM_R.s = (void *)first_cell;
                        DVM_R.m = (void *)other_cell;
                        DVM_R.d = target_cell;
                        DVM_R.PC = &code[0];
                        if (CONTROL == 3)
                        {
                            check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, "integer branch guard refused") &&
                                DVM_R.PC == &code[0] && !memcmp(before, cells, sizeof(cells)), "refusal unchanged");
                            goto finish;
                        }
                        check(kprotect(invoke, NULL) && DVM_R.PC == &code[truth ? 1 : 0] &&
                            !memcmp(before, cells, sizeof(cells)), "pc and unchanged cells");
                        cases++;
                        taken += truth;
                        check(!dvm_native_context_object_count(dvm_native_context_current()), "no leaked guards");
                    }
                }
            }
        }
    }
    check(cases == 101250 && predicates == cases && publications == taken && taken && taken < cases,
        "all eighteen truth/alias cases");
    active = 0;
    for (selected = 0; selected < 18; selected++)
    {
        uvlong x = 1;
        uvlong y = 2;
        unsigned int relation;
        for (relation = 0; relation < 3; relation++)
        {
            int truth;
            void *invalid[3] = {NULL, H, (void *)((uintptr)-1-2)};
            unsigned int bad;
            y = relation;
            truth = expected(&x, &y, selected);
            DVM_R.s = &x;
            DVM_R.m = &y;
            {
                Inst *target = &code[1];
                DVM_R.d = &target;
                DVM_R.PC = &code[0];
                check(dvm_runtime_bytes_fast(&DVM_R, operations[selected]) &&
                    DVM_R.PC == &code[truth ? 1 : 0], "actual branch dispatch");
            }
            for (bad = 0; bad < 3; bad++)
            {
                int completed;
                DVM_R.d = invalid[bad];
                DVM_R.PC = &code[0];
                completed = kprotect(invoke, NULL);
                check((truth ? !completed && !strcmp(getup()->env->errstr, exBounds) : completed) &&
                    DVM_R.PC == &code[0], "untaken target geometry ignored");
            }
            for (bad = 0; bad < 2; bad++)
            {
                Inst *target = bad ? H : NULL;
                int completed;
                DVM_R.d = &target;
                DVM_R.PC = &code[0];
                completed = kprotect(invoke, NULL);
                check((truth ? !completed && !strcmp(getup()->env->errstr, exBounds) : completed) &&
                    DVM_R.PC == &code[0], "untaken target content ignored");
            }
        }
        DVM_R.s = H;
        DVM_R.m = &y;
        DVM_R.d = H;
        DVM_R.PC = &code[0];
        check(!kprotect(invoke, NULL) && !strcmp(getup()->env->errstr, exBounds) &&
            DVM_R.PC == &code[0], "bad source before target");
    }
    {
        Inst *foreign = &code[0];
        const char *invalid = dvm_runtime_integer_branch(MAXDIS, H, H, H, &DVM_R.PC);
        check(invalid && !strcmp(invalid, "integer branch guard refused"), "unknown opcode first");
        invalid = dvm_runtime_integer_branch(IBEQW, H, H, H, &foreign);
        check(invalid && !strcmp(invalid, "integer branch guard refused") &&
            foreign == &code[0], "foreign PC refused");
    }
finish:
    check(!dvm_native_context_object_count(dvm_native_context_current()), "finish unlocked");
    active = 0;
    memcpy(&DVM_R, &saved, sizeof(saved));
    memcpy(getup()->env->errstr, error_text, sizeof(error_text));
    print("PASS branch probe:101250 cases or explicit refusal\n");
}
'''


def frame_guest():
    """Use frame locals for the same eighteen comparisons and score oracle."""
    source = GUEST
    for name, typ in [('b', 'byte'), ('w', 'int'), ('l', 'big')]:
        source = source.replace(f'        {name} := array[2] of {typ};\n', '')
        for index in (0, 1):
            source = source.replace(f'{name}[{index}] = ', f'{name}{index} := ')
            source = source.replace(f'{name}[{index}]', f'{name}{index}')
    return source


def main():
    """Link actual objects, require semantic controls and both C/JIT poll modes."""
    runtime, compiler, include = (Path(x).resolve() for x in sys.argv[1:4])
    with tempfile.TemporaryDirectory(prefix='guarded branch ') as directory:
        work = Path(directory)
        source, image = work/'main.b', work/'main.dis'
        source.write_text(GUEST)
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(image), str(source)], check=True)
        frame_source, frame_image = work/'frame.b', work/'frame.dis'
        frame_source.write_text(frame_guest())
        subprocess.run([str(compiler), '-F', '-I', str(include), '-o', str(frame_image), str(frame_source)], check=True)
        wrapper = work/'wrapper.c'
        wrapper.write_text(WRAPPER)
        for variant in ((0, 3) if '--positive-only' in sys.argv else (0, 1, 2, 3, 4)):
            binary = relink(runtime, work, wrapper, variant,
                ['dvm_runtime_root_registry_probe', 'dvm_managed_small_three_begin',
                 'dvm_runtime_branch_predicate', 'dvm_runtime_branch_target', 'error'])
            for mode in ('--no-jit', '--jit'):
                for locked in (False, True):
                    args = [str(binary), mode, '--gc-worker', '--guarded-copies', '--managed-owner-probe']
                    if variant == 0:
                        args += ['--native-spawn', '--parallel-integer']
                    if locked:
                        args += ['--gc-locked-poll']
                    run = subprocess.run(args+[str(frame_image if variant == 0 else image)], capture_output=True, text=True, timeout=90,
                        env=dict(os.environ, TMPDIR=str(work)))
                    output = run.stdout+run.stderr
                    if 'WARNING: ThreadSanitizer:' in output or 'runtime error:' in output:
                        raise RuntimeError(output)
                    if variant in (1, 2, 4):
                        marker = 'predicate under guards' if variant == 1 else 'pc and unchanged cells'
                        if run.returncode != 10 or 'FAIL branch: '+marker not in output:
                            raise RuntimeError('branch control missed: '+output)
                    elif run.returncode or 'PASS branch probe:' not in output or 'PASS branches18' not in output:
                        raise RuntimeError('branch positive failed: '+output)
                    if variant == 0:
                        leased = {line for line in output.splitlines() if line.startswith('LEASED branch ')}
                        if len(leased) != 18:
                            raise RuntimeError(f'expected 18 leased branch opcodes, saw {sorted(leased)}')
                    if variant == 3 and 'PASS scheduled branch retry' not in output:
                        raise RuntimeError('branch retry missed: '+output)
    print('PASS eighteen actual branches:101250 alias cases, lazy targets, C/JIT guest, both polls')


if __name__ == '__main__':
    main()
