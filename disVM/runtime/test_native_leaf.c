/* Check the production NC64 policy shared by direct and complete native entries. */
#include <stdio.h>
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


/* Record one callback on borrowed registers and return their selected result. */
static int callback(REG *registers)
{
    registers->calls++;
    return registers->result;
}


/* No arguments; return zero after checking refusals, acceptance and both routes.
 * Fixture storage is stack-owned; all failures print a diagnostic and return 1. */
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
    puts("PASS complete/direct NC64 table rejection and fast-only counters");
    return 0;
}
