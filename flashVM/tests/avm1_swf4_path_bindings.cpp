/* SWF4 this/root/parent names remain ordinary variables; modern intrinsic provenance stays private. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "avm1_standard_types.cpp"
#include "../src/standard/movieclip_state.hpp"


static FVM_Value read(FVM_Context *c, FVM_Object *clip, const char *key)
{
    Bytes b;
    text(b, "result");
    global(b, key);
    b.push_back(0x1d);
    b.push_back(0);
    if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), clip) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_object_get(c, clip, "result", &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return out;
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
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *root = 0;
        if (!(fvm_movieclip_create(c, "root", 1, &root) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version == 4)
        {
            const char *names[] = {"this", "_root", "_level0", "_global", "_parent", "_accProps"};
            for (unsigned i = 0; i < 6; ++i)
            {
                if (!(read(c, root, names[i]).type == FVM_VALUE_UNDEFINED))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Bytes b;
                text(b, names[i]);
                char symbol[2] = {(char)('a' + i), 0};
                text(b, symbol);
                b.push_back(0x1d);
                b.push_back(0);
                if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), root) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Value out = read(c, root, names[i]);
                if (!(out.type == FVM_VALUE_STRING && !strcmp(out.string, symbol)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else
        {
            FVM_Value out = read(c, root, "_root");
            if (!(out.type == FVM_VALUE_OBJECT && out.object == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(read(c, root, "_parent").type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Bytes b;
            text(b, "_root");
            text(b, "shadow");
            b.push_back(0x1d);
            b.push_back(0);
            if (!(fvm_avm1_execute_receiver(c, b.begin(), b.size(), root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            out = read(c, root, "_root");
            if (!(out.type == FVM_VALUE_STRING && !strcmp(out.string, "shadow")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_movieclip_preload_display(c, root, false, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
                  out.object == root))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
