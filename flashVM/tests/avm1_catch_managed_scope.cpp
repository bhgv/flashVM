/* Host catch closures retain locals; loaded catches keep ordinary global assignment semantics. */
#define main exceptions_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "actionscript_exceptions.cpp"

#undef main
#include "flashvm/function.h"
#include "flashvm/transfer.h"
#include "../src/core/runtime.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        if (version && version < 4)
        {
            continue;
        }
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (version)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_extensions_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value, out;
        fvm_value_number(&value, 17);
        if (!(fvm_set_global(c, "error", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes body, caught, final, code, closure;
        integer(body, 42);
        body.push_back(0x2a);
        method(caught, "gc", "collect");
        caught.push_back(0x17);
        string(caught, "observed");
        variable(caught, "error");
        caught.push_back(0x1d);
        string(caught, "retained");
        integer(caught, 73);
        caught.push_back(0x3c);
        variable(closure, "retained");
        closure.push_back(0x3e);
        closure.push_back(0);
        define(caught, "capture", closure);
        string(caught, "saved");
        variable(caught, "capture");
        caught.push_back(0x1d);
        string(final, "after");
        variable(final, "error");
        final.push_back(0x1d);
        attempt(code, body, caught, final, 3, "error");
        run(c, code, FVM_OK);
        if (!(global(c, "observed") == 42 && global(c, "after") == (version ? 42 : 17) &&
              global(c, "error") == (version ? 42 : 17) && c->lexical_scopes.empty() && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_get_global(c, "saved", &value) == FVM_OK &&
              value.type == FVM_VALUE_OBJECT && fvm_function_invoke(c, value.object, 0, 0, 0, &out) == FVM_OK &&
              out.number == 73))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Context *other = 0;
        if (!(fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            if (!(fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_extensions_install(other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 91);
        if (!(fvm_set_global(other, "retained", &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Channel *channel = 0;
        if (!(fvm_channel_create(&channel) == FVM_OK && fvm_channel_send_value(c, channel, &value, 0) == FVM_OK &&
              fvm_channel_receive_value(other, channel, &value, 0) == FVM_OK &&
              fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK &&
              fvm_function_invoke(other, value.object, 0, 0, 0, &out) == FVM_OK && out.number == (version ? 91 : 73)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_channel_destroy(channel);
        fvm_context_destroy(other);
        if (!(c->lexical_scopes.empty() && !c->gc_frames))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
