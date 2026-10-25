/* Frame Call/GotoFrame2 empty inputs use versioned labels; malformed payloads still fail first. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"


static void run(FVM_Context *c, FVM_Object *root, Bytes code)
{
    code.push_back(0);
    if (!(fvm_avm1_execute_receiver(c, code.begin(), code.size(), root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 6; ++mode)
    {
        FVM_Context *c = 0;
        FVM_Object *root = 0;
        unsigned version = mode + 3;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 3, &root) == FVM_OK &&
              fvm_gc_pin(c, root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0};
        Bytes body;
        text(body, "seen");
        integer(body, 3);
        body.push_back(0x1d);
        body.push_back(0);
        if (!(fvm_movieclip_define_frame(c, root, 1, "first", empty, sizeof(empty)) == FVM_OK &&
              fvm_movieclip_define_frame(c, root, 2, "second", empty, sizeof(empty)) == FVM_OK &&
              fvm_movieclip_define_frame(c, root, 3, "undefined", body.begin(), body.size()) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Bytes call;
        call.push_back(0x9e);
        u16(call, 0);
        call.push_back(0);
        FVM_Value out;
        FVM_Result r = fvm_avm1_execute_receiver(c, call.begin(), call.size(), root);
        if (!mode)
        {
            if (!(r == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_object_get(c, root, "seen", &out) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version >= 7)
            {
                if (!(out.number == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(out.type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        for (unsigned bias = 0; bias < 2; ++bias)
        {
            Bytes code;
            code.push_back(0x81);
            u16(code, 2);
            u16(code, 0);
            code.push_back(0x9f);
            u16(code, bias ? 3 : 1);
            code.push_back(bias ? 2 : 0);
            if (bias)
            {
                u16(code, 65535);
            }
            code.push_back(0);
            r = fvm_avm1_execute_receiver(c, code.begin(), code.size(), root);
            if (!mode)
            {
                if (!(r == FVM_ERROR))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(r == FVM_OK && fvm_object_get(c, root, "_currentframe", &out) == FVM_OK &&
                      out.number == (version >= 7 ? 3 : 1)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (mode)
        {
            Bytes code;
            text(code, "002");
            code.push_back(0x9f);
            u16(code, 3);
            code.push_back(2);
            u16(code, 1);
            run(c, root, code);
            if (!(fvm_object_get(c, root, "_currentframe", &out) == FVM_OK && out.number == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        const unsigned char bad_call[] = {0x9e, 1, 0, 0, 0}, bad_goto[] = {0x9f, 1, 0, 4, 0};
        if (!(fvm_avm1_execute_receiver(c, bad_call, sizeof(bad_call), root) == FVM_INVALID_ARGUMENT &&
              fvm_avm1_execute_receiver(c, bad_goto, sizeof(bad_goto), root) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
