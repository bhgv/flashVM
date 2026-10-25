#define FVM_CONVOLUTION_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "convolution_filter.cpp"


static inline void
noise_args(FVM_Value *a, double seed, double low = 0, double high = 255, double channels = 7, bool gray = false)
{
    fvm_value_number(a, seed);
    fvm_value_number(a + 1, low);
    fvm_value_number(a + 2, high);
    fvm_value_number(a + 3, channels);
    fvm_value_boolean(a + 4, gray);
}


static inline void noise_packet(FVM_Context *c, FVM_Object *image)
{
    if (!(fvm_gc_pin(c, image) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value a[5];
    noise_args(a, 1);
    call(c, image, "noise", 1, a);
    if (!(read(c, image, 0, 0) == 0xffa7f1d9UL && read(c, image, 1, 0) == 0xff2a82c8UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Result noise_dispose_seed(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *u)
{
    FVM_Value ignored;
    if (!(fvm_object_call(c, (FVM_Object *)u, "dispose", 0, 0, &ignored) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}
#ifndef FVM_NOISE_FIXTURE_ONLY


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0, *other = 0;
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
    FVM_Object *image = bitmap(c, 2, 1, true, 0);
    FVM_Value a[5];
    noise_args(a, 1);
    unsigned long random_before = c->random_state;
    call(c, image, "noise", 1, a);
    if (!(read(c, image, 0, 0) == 0xffa7f1d9UL && read(c, image, 1, 0) == 0xff2a82c8UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->random_state == random_before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    call(c, image, "noise", 1, a);
    if (!(read(c, image, 1, 0) == 0xff2a82c8UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double seeds[] = {0, 1, -2147483647.0};
    for (unsigned i = 0; i < 3; ++i)
    {
        noise_args(a, seeds[i]);
        call(c, image, "noise", 1, a);
        if (!(read(c, image, 0, 0) == 0xffa7f1d9UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    const double twos[] = {-1, 4294967295.0, -2147483648.0, 2147483648.0};
    for (unsigned i = 0; i < 4; ++i)
    {
        noise_args(a, twos[i]);
        call(c, image, "noise", 1, a);
        if (!(read(c, image, 0, 0) == 0xff4ee2b3UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    noise_args(a, 2147483647.0);
    call(c, image, "noise", 1, a);
    if (!(read(c, image, 0, 0) == 0xff000000UL && read(c, image, 1, 0) == 0xff000000UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_args(a, std::numeric_limits<double>::infinity());
    call(c, image, "noise", 1, a);
    if (!(read(c, image, 0, 0) == 0xffa7f1d9UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned masks[] = {0, 1, 2, 4, 5, 8, 15};
    const unsigned long expected_first[] = {
        0xff000000UL, 0xffa70000UL, 0xff00a700UL, 0xff0000a7UL, 0xffa700f1UL, 0xa7000000UL, 0x2aa7f1d9UL};
    const unsigned long expected_second[] = {
        0xff000000UL, 0xfff10000UL, 0xff00f100UL, 0xff0000f1UL, 0xffd9002aUL, 0xf1000000UL, 0xfe82c8d8UL};
    for (unsigned i = 0; i < 7; ++i)
    {
        noise_args(a, 1, 0, 255, masks[i]);
        call(c, image, "noise", 4, a);
        if (!(read(c, image, 0, 0) == expected_first[i] && read(c, image, 1, 0) == expected_second[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    noise_args(a, 1, 0, 255, 0, true);
    call(c, image, "noise", 5, a);
    if (!(read(c, image, 0, 0) == 0xffa7a7a7UL && read(c, image, 1, 0) == 0xfff1f1f1UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_args(a, 1, 0, 255, 15, true);
    call(c, image, "noise", 5, a);
    if (!(read(c, image, 0, 0) == 0xf1a7a7a7UL && read(c, image, 1, 0) == 0x2ad9d9d9UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *opaque = bitmap(c, 2, 1, false, 0);
    noise_args(a, 1, 0, 255, 15);
    call(c, opaque, "noise", 5, a);
    if (!(read(c, opaque, 0, 0) == 0xffa7f1d9UL && read(c, opaque, 1, 0) == 0xff2a82c8UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *opaque_reference = bitmap(c, 17, 7, false, 0), *opaque_masks = bitmap(c, 17, 7, false, 0);
    for (unsigned mask = 0; mask < 16; ++mask)
    {
        for (unsigned gray = 0; gray < 2; ++gray)
        {
            noise_args(a, 1, 10, 230, mask, gray != 0);
            call(c, opaque_masks, "noise", 5, a);
            noise_args(a, 1, 10, 230, mask & 7, gray != 0);
            call(c, opaque_reference, "noise", 5, a);
            for (unsigned y = 0; y < 7; ++y)
            {
                for (unsigned x = 0; x < 17; ++x)
                {
                    if (!(read(c, opaque_masks, x, y) == read(c, opaque_reference, x, y)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    noise_args(a, 1, 200, 100, 15);
    call(c, image, "noise", 5, a);
    if (!(read(c, image, 0, 0) == 0xc8c8c8c8UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_args(a, 1, -1, 0);
    call(c, image, "noise", 5, a);
    if (!(read(c, image, 0, 0) == 0xffffffffUL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_args(a, 1, 0, 0, 15);
    call(c, image, "noise", 5, a);
    if (!(read(c, image, 0, 0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_args(a, 1, 0, 255, 23);
    call(c, image, "noise", 5, a);
    if (!(read(c, image, 0, 0) == 0xffa7f1d9UL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *many = bitmap(c, 17, 7, true, 0);
    const double oracle_seeds[] = {1, 2, 2147483646.0, 2147483647.0, 2147483648.0, -2147483648.0, 4294967295.0};
    for (unsigned seed_index = 0; seed_index < 7; ++seed_index)
    {
        noise_args(a, oracle_seeds[seed_index]);
        call(c, many, "noise", 1, a);
        double signed_seed = oracle_seeds[seed_index];
        if (signed_seed >= 2147483648.0)
        {
            signed_seed -= 4294967296.0;
        }
        double state = signed_seed <= 0 ? 1 - signed_seed : signed_seed;
        for (unsigned y = 0; y < 7; ++y)
        {
            for (unsigned x = 0; x < 17; ++x)
            {
                unsigned long color = 0xff000000UL;
                for (unsigned channel = 0; channel < 3; ++channel)
                {
                    state = std::fmod(state * 16807.0, 2147483647.0);
                    color |= (unsigned long)std::fmod(state, 256.0) << (16 - channel * 8);
                }
                if (!(read(c, many, x, y) == color))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }
    unsigned long before = (unsigned long)read(c, image, 0, 0);
    call(c, image, "noise");
    if (!(read(c, image, 0, 0) == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long previous = rt->config.memory_limit_bytes;
    rt->config.memory_limit_bytes = 7;
    noise_args(a, 1);
    FVM_Value out;
    if (!(fvm_object_call(c, image, "noise", 5, a, &out) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.memory_limit_bytes = previous;
    if (!(read(c, image, 0, 0) == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Channel *ch = 0;
    FVM_Value value = ref(image), copy;
    if (!(fvm_channel_create(&ch) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_packet(other, copy.object);
    fvm_channel_destroy(ch);
    FVM_Result opened = fvm_channel_open_file("noise-test.bin", "w", &ch);
    if (opened == FVM_OK)
    {
        if (!(fvm_channel_send_value(c, ch, &value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        if (!(fvm_channel_open_file("noise-test.bin", "r", &ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_receive_value(other, ch, &copy, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        noise_packet(other, copy.object);
        if (!(std::remove("noise-test.bin") == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(opened == FVM_UNSUPPORTED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#ifdef OS_PORTS
    FVM_ResourceBroker *broker = 0;
    FVM_Channel *queue = 0, *network = 0;
    FVM_Object *wrapped = 0;
    char *uri = 0;
    if (!(fvm_broker_start("127.0.0.1", "127.0.0.1", 0, &broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_set_broker(c, broker) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_create(&queue) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_extensions_wrap_channel(c, queue, &wrapped) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_broker_export(c, wrapped, &uri) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_open_network(other, uri, &network) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_send_value(c, queue, &value, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_channel_receive_value(other, network, &copy, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_packet(other, copy.object);
    fvm_channel_release(network);
    fvm_broker_stop(broker);
    fvm_channel_release(queue);
    fvm_broker_release(broker);
    fvm_serialized_free(uri);
#endif
    FVM_Object *disposed = bitmap(c, 2, 1, true, 0), *seed_object = 0;
    if (!(fvm_object_create(c, &seed_object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_pin(c, seed_object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, seed_object, "valueOf", noise_dispose_seed, disposed, disposed) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    noise_args(a, 1);
    a[0] = ref(seed_object);
    if (!(call(c, disposed, "noise", 5, a).number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(call(c, disposed, "noise").number == -1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(other);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
