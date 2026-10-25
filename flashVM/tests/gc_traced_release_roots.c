#include <stdio.h>
#include <stdlib.h>
#include "flashvm/gc.h"
#include "flashvm/stack.h"
#include "flashvm/userdata.h"
#include <string.h>


typedef struct Payload
{
    FVM_Stack *stack;
    FVM_Value value;
    unsigned released;
} Payload;


static void release(void *raw)
{
    Payload *p = (Payload *)raw;
    ++p->released;
    if (!(!memcmp(p->value.string, "A\0B", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stack_push(p->stack, &p->value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *o;
    FVM_Value v;
    FVM_GCStringStats stats;
    Payload payload;
    unsigned short text[3] = {'A', 0, 'B'};
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stack_create(c, &payload.stack) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    payload.released = 0;
    if (!(fvm_object_create(c, &o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_utf16(c, text, 3, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, o, "value", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, o, "value", &payload.value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_userdata_attach(c, o, &payload, release) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && payload.released == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_stats(c, &stats) == FVM_OK && stats.buffers == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stack_get(payload.stack, 1, &v) == FVM_OK && !memcmp(v.string, "A\0B", 3)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stack_clear(payload.stack) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_string_stats(c, &stats) == FVM_OK && !stats.buffers))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_stack_destroy(payload.stack);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
