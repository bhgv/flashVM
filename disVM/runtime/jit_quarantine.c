/* Copyright (c) 2026 bhgv
 * EN: Immutable bounded function quarantine records, published without stopping VM threads.
 * RU: Immutable bounded записи карантина функций без остановки потоков VM.
 */
#include "lib9.h"
#include "interp.h"
#include "isa.h"
#include "jit_publication.h"
#include "jit_quarantine.h"
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "raise.h"

#define QUARANTINE_MAX 128 /* Bounded exceptional metadata, not an execution/object catalog. */


/* EN/RU: Published immutable function interval, scoped to a unique JIT lifetime.
 * Опубликованный immutable интервал функции в unique lifetime JIT. */
typedef struct Quarantine
{
    unsigned long generation; /* Never reused, including unload/reload at the same address. */
    int start;                /* Inclusive original Dis function entry. */
    int end;                  /* Exclusive next function entry or module end. */
} Quarantine;

static Quarantine records[QUARANTINE_MAX]; /* Native scalar metadata only. */
static unsigned int published;             /* Release-published immutable prefix length. */
static int exhausted;                      /* Fail closed to interpreter after record capacity is exhausted. */
static unsigned long generation;           /* Atomic unique generation source; zero is unavailable. */
static pthread_mutex_t writer = PTHREAD_MUTEX_INITIALIZER; /* Exceptional publication only. */
static __thread unsigned long selected;                    /* Scalar generation of the selected native entry. */
static __thread const void *selected_frame;                /* Identity comparison only, never dereferenced. */
static __thread int first;                                 /* Cached inclusive function entry. */
static __thread int last;                                  /* Cached exclusive function limit. */
static __thread int current_opcode;                        /* Original opcode copied before native entry. */
static __thread int current_pc;                            /* Exact admitted original Dis PC, not native RX address. */
static __thread int native_active;                         /* Set only while a selected native callback runs. */
static __thread char module_name[256];                     /* Copied path for diagnostics, no retained Module. */


/* EN/RU: Allocate nonrepeating native generation; permanent exhaustion is explicit.
 * Выдать неповторяемую native generation; исчерпание необратимо и явно. */
unsigned long dvm_jit_new_generation(void)
{
    unsigned long before = __atomic_load_n(&generation, __ATOMIC_RELAXED);
    while (before != ULONG_MAX)
    {
        if (__atomic_compare_exchange_n(&generation, &before, before + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return before + 1;
        }
    }
    return 0;
}


/* [GC:] EN/RU: Refine the interval around pc using a checked immutable entry pointer.
 * Уточнить интервал вокруг pc по проверенному immutable entry pointer. */
static void boundary(Module *module, const Inst *entry, int pc)
{
    uintptr base = (uintptr)module->prog;
    uintptr address = (uintptr)entry;
    uintptr offset;
    int index;
    if (address < base || !entry || entry == H)
    {
        return;
    }
    offset = address - base;
    if (offset % sizeof(Inst) || offset / sizeof(Inst) >= (uintptr)module->nprog)
    {
        return;
    }
    index = (int)(offset / sizeof(Inst));
    if (index <= pc && index > first)
    {
        first = index;
    }
    if (index > pc && index < last)
    {
        last = index;
    }
}


/* [GC:] EN: Borrow rooted immutable module until returning; scalar cache contains
 * no ownership. Local calls/spawns, module entry and export entries partition code.
 * If geometry is unavailable, decline JIT. Other functions/modules stay native.
 * RU: Заимствовать rooted immutable module до return; scalar cache не владеет им.
 * Local calls/spawns, module entry и exports задают границы code. При неверной
 * geometry отказать JIT. Другие функции/modules продолжают native execution. */
Jit *dvm_jit_select(Module *module, const void *next_pc, const void *frame)
{
    Jit *jit = dvm_jit_load(module);
    uintptr offset;
    unsigned int count;
    unsigned int i;
    int pc;
    if (jit == H || (jit && (uintptr)jit > (uintptr)-1 - sizeof(*jit)))
    {
        selected = 0;
        return jit; /* Existing checked dispatch raises exCompile before a callback. */
    }
    if (!jit || !jit->generation || __atomic_load_n(&exhausted, __ATOMIC_ACQUIRE) || !module->prog ||
        module->nprog <= 0 || (uintptr)next_pc <= (uintptr)module->prog)
    {
        selected = 0;
        return NULL;
    }
    offset = (uintptr)next_pc - (uintptr)module->prog;
    if (offset % sizeof(Inst) || offset / sizeof(Inst) > (uintptr)module->nprog)
    {
        selected = 0;
        return NULL;
    }
    pc = (int)(offset / sizeof(Inst)) - 1;
    if (selected != jit->generation || selected_frame != frame || pc < first || pc >= last)
    {
        Link *link;
        first = 0;
        last = module->nprog;
        boundary(module, module->entry, pc);
        for (i = 0; i < (unsigned int)module->nprog; i++)
        {
            Inst *instruction = module->prog + i;
            if (instruction->op == ICALL || instruction->op == ISPAWN)
            {
                boundary(module, instruction->d.ins, pc);
            }
        }
        for (link = module->ext; link && link->name; link++)
        {
            boundary(module, link->u.pc, pc);
        }
        snprintf(module_name, sizeof(module_name), "%s", module->path ? module->path : "?");
    }
    selected = jit->generation;
    selected_frame = frame;
    current_pc = pc;
    current_opcode = module->prog[pc].op;
    count = __atomic_load_n(&published, __ATOMIC_ACQUIRE);
    for (i = 0; i < count; i++)
    {
        if (records[i].generation == selected && pc >= records[i].start && pc < records[i].end)
        {
            return NULL;
        }
    }
    return jit;
}


/* EN: Quarantine the entire selected function for a confirmed native defect.
 * Called before unwinding; no current instruction is retried. Ordinary managed
 * throws/unsupported fast paths never call this API. Metadata never retains code.
 * RU: Карантин всей selected функции при подтверждённом native defect. Вызов до
 * unwind; текущая instruction не повторяется. Обычные managed throws/unsupported
 * fast paths API не вызывают. Metadata не удерживает code. */
int dvm_jit_report_failure(int opcode, const char *cause)
{
    unsigned int count;
    unsigned int i;
    if (!selected)
    {
        return 0;
    }
    pthread_mutex_lock(&writer);
    count = __atomic_load_n(&published, __ATOMIC_RELAXED);
    for (i = 0; i < count; i++)
    {
        if (records[i].generation == selected && records[i].start == first && records[i].end == last)
        {
            pthread_mutex_unlock(&writer);
            return 1;
        }
    }
    if (count < QUARANTINE_MAX)
    {
        records[count].generation = selected;
        records[count].start = first;
        records[count].end = last;
        __atomic_store_n(&published, count + 1, __ATOMIC_RELEASE);
    }
    else
    {
        __atomic_store_n(&exhausted, 1, __ATOMIC_RELEASE);
    }
    fprintf(
        stderr,
        "JITFAULT generation=%lu module=%s function=[%d,%d) pc=%d opcode=%d backend=%s cause=%s action=interpret-function replay=0 capacity_exhausted=%d\n",
        selected,
        module_name,
        first,
        last,
        current_pc,
        opcode < 0 ? current_opcode : opcode,
#if defined(__aarch64__)
        "arm64",
#elif defined(__riscv)
        "riscv64",
#else
        "x64",
#endif
        cause ? cause : "confirmed backend defect",
        count == QUARANTINE_MAX);
    pthread_mutex_unlock(&writer);
    return 1;
}


/* EN/RU: Mark selected scalar provenance as executing native code on this owner.
 * Отметить selected scalar provenance как executing native code своего owner. */
void dvm_jit_native_begin(void)
{
    native_active = selected != 0;
}


/* EN/RU: Normal return clears native exception attribution, including C fallback.
 * Normal return снимает native exception attribution, включая C fallback. */
void dvm_jit_native_end(void)
{
    native_active = 0;
}


/* EN/RU: Classify explicit backend exception; normal nil/divide/bounds throws do
 * not quarantine code. No retry, register repair, callbacks or signal unwinding.
 * Классифицировать explicit backend exception; nil/divide/bounds не карантинят
 * code. Нет retry, исправления REG, callbacks либо signal unwinding. */
int dvm_jit_native_error(const char *message)
{
    int active = native_active;
    native_active = 0;
    if (active && message && !strcmp(message, exCompile))
    {
        return dvm_jit_report_failure(-1, "native callback raised compile exception");
    }
    return 0;
}
