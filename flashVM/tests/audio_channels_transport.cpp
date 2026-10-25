/* Transported stopAllSounds retains native callback identity and affects only the destination. */
#define main audio_channels_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "audio_channels.c"

#undef main
#include "flashvm/function.h"
#include "flashvm/serialization.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *source = 0, *target = 0;
    Host a, b;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &source) == FVM_OK && fvm_context_create(rt, &target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(source) == FVM_OK && fvm_standard_install(target) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    create(source, &a);
    create(target, &b);
    unsigned char data[8] = {0};
    FVM_MediaFrame frame;
    memset(&frame, 0, sizeof(frame));
    frame.kind = FVM_MEDIA_AUDIO;
    frame.data = data;
    frame.size = sizeof(data);
    frame.channels = 2;
    frame.sample_rate = 8000;
    frame.samples = 2;
    if (!(fvm_audio_channel_write(a.channel, &frame) == FVM_OK && fvm_audio_channel_write(b.channel, &frame) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value function, copy, out;
    unsigned char *wire = 0;
    size_t length = 0;
    if (!(fvm_get_global(source, "stopAllSounds", &function) == FVM_OK && function.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_serialize(source, &function, &wire, &length) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_deserialize(target, wire, length, &copy) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_serialized_free(wire);
    if (!(fvm_function_invoke(target, copy.object, 0, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!a.stops && b.stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(a.channel) == FVM_OK && a.stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_close(b.channel) == FVM_OK && b.stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(source);
    fvm_context_destroy(target);
    fvm_runtime_destroy(rt);
    return 0;
}
