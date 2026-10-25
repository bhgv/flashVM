/* Receiver libraries start fresh after transfer/source destruction, then completed tables cost no work. */
#include <stdio.h>
#include <stdlib.h>
#include "initializer_progress_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        FVM_Context *c = tick_context(rt, version), *other = tick_context(rt, version);
        FVM_Object *root = init_root(c, version, 3);
        InitProbe a;
        init_bind(c, root, a);
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && a.library->swf_initializer_next == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        /* Native method registrations do not cross graph transport; the global binding is local too. */
        FVM_Channel *ch = 0;
        FVM_Value packet = tick_object(root), out;
        if (!(fvm_channel_create(&ch) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_channel_send_value(c, ch, &packet, 0) == FVM_OK &&
              fvm_channel_receive_value(other, ch, &out, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        root = out.object;
        if (!(fvm_gc_pin(other, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(ch);
        fvm_context_destroy(c);
        InitProbe b;
        init_bind(other, root, b);
        if (!(!b.library->swf_initializer_next))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_string_mode(other, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_advance(other, root, 1) == FVM_OK && b.calls[0] == 1 && b.calls[1] == 1 && b.calls[2] == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        rt->config.instruction_limit = 2;
        if (!(fvm_movieclip_advance(other, root, 1) == FVM_OK && other->avm1_steps == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        init_clean(other, b);
        rt->config.instruction_limit = 0;
        fvm_context_destroy(other);
    }
    fvm_runtime_destroy(rt);
}
