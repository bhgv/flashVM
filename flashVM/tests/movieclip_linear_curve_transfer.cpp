#include <stdio.h>
#include <stdlib.h>
#include "linear_curve_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned kind = 0; kind < 24; ++kind)
    {
        FVM_Context *sender = 0, *binary = 0, *channel = 0;
        ss_context(rt, &sender);
        ss_context(rt, &binary);
        ss_context(rt, &channel);
        FVM_Object *clip = lc_clip(sender, (kind / 2) % 3, kind % 2, 4, kind / 6);
        ss_xy(sender, clip, "moveTo", 100, 100);
        FVM_Value value, copy;
        ref(&value, clip);
        unsigned char *bytes = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(sender, &value, &bytes, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_deserialize(binary, bytes, size, &copy) == FVM_OK))
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
        if (!(fvm_gc_collect(binary, 0) == FVM_OK && fvm_gc_collect(channel, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        lc_expected(rb_bounds(binary, b), (kind / 2) % 3);
        lc_expected(rb_bounds(channel, q), (kind / 2) % 3);
        fvm_context_destroy(binary);
        fvm_context_destroy(channel);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
