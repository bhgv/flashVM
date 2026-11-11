/* Copyright (c) 2026 bhgv
 * EN: Check production NC64 policy and exact accepted counts across native threads.
 * RU: Проверить NC64-политику и точный учёт принятых операций в native-потоках.
 */
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include "isa.h"


/* Fixture state is borrowed by callbacks; only calls/result are observable. */
typedef struct REG
{
    int calls;  /* Number of actual native callback invocations. */
    int result; /* Callback result, including nonzero negative completion. */
} REG;


/* Only the production policy's borrowed fast-table field is needed here. */
typedef struct Jit
{
    int (*entry[256])(REG *); /* Selected callbacks indexed by decoded opcode. */
} Jit;

static unsigned long native_instructions; /* Same counter owned by main.c. */
#include "native_leaf.c"


/* EN: Record one callback on borrowed registers and return their selected result.
 * RU: Учесть callback в заимствованных registers и вернуть заданный результат. */
static int callback(REG *registers)
{
    registers->calls++;
    return registers->result;
}


static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER; /* Parallel counter fixture only. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Common start predicate. */
static unsigned ready; /* Readers at initial gate, protected by gate. */
static int started; /* Main releases all native callers, protected by gate. */


/* EN: Require condition or terminate with borrowed diagnostic message.
 * RU: Проверить condition либо завершить с заимствованным диагностическим текстом. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL native acceptance counter: %s\n", message);
        exit(12);
    }
}


/* EN: Exercise direct/leaf, accepted/declined/forbidden routes with private REG.
 * unused is NULL; jit and registers remain local for all 120000 calls.
 * RU: Проверить direct/leaf, принятые/отказанные/запрещённые пути с личным REG.
 * unused равен NULL; jit и регистры личные для всех 120000 вызовов. */
static void *parallel(void *unused)
{
    (void)unused;
    Jit jit;
    unsigned round;
    memset(&jit, 0, sizeof(jit));
    jit.entry[IADDW] = callback;
    jit.entry[ILOAD] = callback;
    jit.entry[ICASE] = callback;
    jit.entry[IGOTO] = callback;
    pthread_mutex_lock(&gate);
    ready++;
    pthread_cond_broadcast(&changed);
    while (!started)
    {
        pthread_cond_wait(&changed, &gate);
    }
    pthread_mutex_unlock(&gate);
    for (round = 0; round < 120000; round++)
    {
        int variant = round % 6;
        int operation = variant == 3 ? ICASE : variant == 5 ? IGOTO : variant == 4 ? ILOAD : IADDW;
        int result = variant == 1 ? 0 : variant == 2 ? -7 : 1;
        int expected = variant == 3 || variant == 5 ? 0 : result;
        REG registers = {0, result};
        int actual = round % 2 ? dvm_runtime_leaf(&jit, operation, &registers) :
            dvm_runtime_fast(operation, callback, &registers);
        require(actual == expected && registers.calls == (variant == 3 || variant == 5 ? 0 : 1),
            "parallel callback policy and private registers remain exact");
    }
    return NULL;
}


/* EN: Count 480000 genuine accepted callbacks from eight concurrent native callers.
 * RU: Учесть ровно 480000 принятых callbacks восьми параллельных native-вызывателей. */
static void parallel_count(void)
{
    pthread_t threads[8];
    unsigned index;
    native_instructions = 0;
    for (index = 0; index < 8; index++)
    {
        require(!pthread_create(&threads[index], NULL, parallel, NULL), "native caller starts");
    }
    pthread_mutex_lock(&gate);
    while (ready != 8)
    {
        pthread_cond_wait(&changed, &gate);
    }
    started = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&gate);
    for (index = 0; index < 8; index++)
    {
        require(!pthread_join(threads[index], NULL), "native caller joins");
    }
    require(__atomic_load_n(&native_instructions, __ATOMIC_RELAXED) == 480000,
        "accepted native instruction statistics never lose concurrent increments");
    puts("PASS native parallel acceptance calls=960000 accepted=480000 workers=8");
}


/* EN: Check serial policy and parallel acceptance totals; no arguments, return zero
 * on success. Fixture storage is owned until join; failures report and terminate.
 * RU: Проверить последовательную политику и параллельный счётчик; без аргументов,
 * успех возвращает ноль. Память принадлежит тесту до join; сбои завершают тест. */
int main(void)
{
    static const int operations[] = {ICASE, ICASEL, IGOTO, IADDW, ILOAD};
    static const int results[] = {0, 1, -7};
    Jit jit;
    unsigned i;
    unsigned j;
    int route;
    memset(&jit, 0, sizeof(jit));
    for (i = 0; i < sizeof(operations) / sizeof(operations[0]); ++i)
    {
        int operation = operations[i];
        jit.entry[operation] = callback;
        for (j = 0; j < sizeof(results) / sizeof(results[0]); ++j)
        {
            for (route = 0; route < 2; ++route)
            {
                REG registers = {0, results[j]};
                int expected = i < 3 ? 0 : results[j];
                int actual;
                native_instructions = 0;
                actual = route ? dvm_runtime_leaf(&jit, operation, &registers)
                               : dvm_runtime_fast(operation, callback, &registers);
                if (actual != expected || registers.calls != (i < 3 ? 0 : 1) ||
                    native_instructions != (unsigned long)(expected != 0))
                {
                    fprintf(stderr,
                            "NC64 JIT policy mismatch: operation=%d route=%d result=%d\n",
                            operation,
                            route,
                            results[j]);
                    return 1;
                }
            }
        }
    }
    parallel_count();
    puts("PASS complete/direct NC64 table rejection and fast-only counters");
    return 0;
}
