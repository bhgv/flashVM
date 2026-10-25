#include <stdio.h>
#include <stdlib.h>
#include "secondary_axis_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned n = 0; n < 48; ++n)
    {
        FVM_Context *sender = 0, *binary = 0, *channel = 0;
        ss_context(rt, &sender);
        ss_context(rt, &binary);
        ss_context(rt, &channel);
        int e = n % 2 ? -600 : 10;
        unsigned cap = (n / 2) % 3, join = (n / 6) % 2, kind = n / 12;
        FVM_Object *clip = sa_clip(sender, cap, join, e, kind);
        FVM_Value value, copy;
        ref(&value, clip);
        unsigned char *bytes = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(sender, &value, &bytes, &size) == FVM_OK &&
              fvm_value_deserialize(binary, bytes, size, &copy) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(bytes);
        FVM_Object *b = copy.object;
        if (!(fvm_gc_pin(binary, b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Channel *ch = 0;
        if (!(fvm_channel_create(&ch) == FVM_OK && fvm_channel_send_value(sender, ch, &value, 0) == FVM_OK &&
              fvm_channel_receive_value(channel, ch, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *q = copy.object;
        if (!(fvm_gc_pin(channel, q) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        fvm_context_destroy(sender);
        FVM_Object *bt = sa_target(binary), *qt = sa_target(channel);
        if (!(fvm_gc_collect(binary, 0) == FVM_OK && fvm_gc_collect(channel, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        sa_expected(sa_bounds(binary, b, bt, e), cap);
        sa_expected(sa_bounds(channel, q, qt, e), cap);
        fvm_context_destroy(binary);
        fvm_context_destroy(channel);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
