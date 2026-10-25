#include <stdio.h>
#include <stdlib.h>
#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/metamethod.h"
#include "flashvm/standard.h"
#include "flashvm/transfer.h"


static void value(FVM_Value *v, FVM_Object *o)
{
    fvm_value_undefined(v);
    v->type = FVM_VALUE_OBJECT;
    v->object = o;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
    FVM_Object *owner = 0, *meta = 0, *fn = 0;
    FVM_Channel *channel = 0;
    FVM_Value v, out, copy;
    const unsigned char constant[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    const unsigned char finalized[] = {0x96, 11, 0,    0, 'f', 'i', 'n', 'a', 'l', 'i', 'z',  'e',
                                       'd',  0,  0x96, 5, 0,   7,   42,  0,   0,   0,   0x1d, 0};
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
    if (!(fvm_context_create(rt, &other) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_construct(c, "Object", 0, 0, &meta) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, constant, sizeof(constant), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value(&v, fn);
    if (!(fvm_object_set(c, meta, "__index", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, meta, "__add", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, finalized, sizeof(finalized), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value(&v, fn);
    if (!(fvm_object_set(c, meta, "__gc", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value(&v, owner);
    if (!(fvm_value_set_metatable(c, &v, meta) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, owner, "missing", &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_set_global(c, "owner", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    {
        const unsigned char add[] = {0x96, 8, 0, 0, 'r', 'e', 's', 'u', 'l',  't',  0,
                                     0x96, 7, 0, 0, 'o', 'w', 'n', 'e', 'r',  0,    0x1c,
                                     0x96, 5, 0, 7, 7,   0,   0,   0,   0x47, 0x1d, 0};
        if (!(fvm_avm1_execute(c, add, sizeof(add)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "result", &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_channel_create(&channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, channel, &v, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, channel, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(other, copy.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(other, copy.object, "missing", &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&v, 9);
    if (!(fvm_value_meta_invoke(other, &copy, "__add", 1, &v, &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(other, copy.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(other, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_get_global(other, "finalized", &out) == FVM_OK && out.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_channel_destroy(channel);
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
