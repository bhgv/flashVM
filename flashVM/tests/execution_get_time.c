/* Deterministic link-time clock proves context origins, unsigned wrap and failure atomicity. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/avm1.h"
#include "../ports/monotonic_port.h"
#include <math.h>

static double sample;
static int available = 1;
static unsigned reads;


int fvm_port_monotonic_ms(double *output)
{
    ++reads;
    if (!available || !output)
    {
        return 0;
    }
    *output = sample;
    return 1;
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 4; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Value out;
        unsigned before;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        const unsigned char actions[] = {0x96, 6, 0, 0, 't', 'i', 'c', 'k', 0, 0x34, 0x1d, 0};
        swf[3] = (unsigned char)version;
        sample = 1000.25;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1042.99;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_avm1_execute(c, actions, sizeof(actions)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "tick", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1234.99;
        if (!(fvm_exec_get_time(c, &out) == FVM_OK && out.number == 234))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1100;
        if (!(fvm_exec_get_time(c, &out) == FVM_OK && out.number == 234))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1000.25 + 2147483648.99;
        if (!(fvm_exec_get_time(c, &out) == FVM_OK && out.number == 2147483648.0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1000.25 + 4294967296.75;
        if (!(fvm_exec_get_time(c, &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1000.25 + 4294967297.25;
        if (!(fvm_exec_get_time(c, &out) == FVM_OK && out.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        available = 0;
        fvm_value_number(&out, 99);
        if (!(fvm_exec_get_time(c, &out) == FVM_ERROR && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        available = 1;
        sample = HUGE_VAL;
        if (!(fvm_exec_get_time(c, &out) == FVM_ERROR && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = -1;
        if (!(fvm_exec_get_time(c, &out) == FVM_ERROR && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 1000;
        if (!(fvm_exec_get_time(c, &out) == FVM_OK && out.number == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        before = reads;
        if (!(fvm_exec_get_time(0, &out) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_get_time(c, 0) == FVM_INVALID_ARGUMENT && reads == before))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    {
        FVM_Context *a, *b;
        FVM_Value out;
        available = 0;
        if (!(fvm_context_create(rt, &a) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_get_time(a, &out) == FVM_ERROR && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        available = 1;
        sample = 2000;
        if (!(fvm_exec_get_time(a, &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 2005.99;
        if (!(fvm_exec_get_time(a, &out) == FVM_OK && out.number == 5))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 2100;
        if (!(fvm_context_create(rt, &b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sample = 2200;
        if (!(fvm_exec_get_time(a, &out) == FVM_OK && out.number == 200))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_get_time(b, &out) == FVM_OK && out.number == 100))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(b);
        fvm_context_destroy(a);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
