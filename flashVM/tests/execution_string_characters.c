/* C89 string/character services preserve UTF-16, legacy limits, aliases and callback roots. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include <string.h>
#include <math.h>

static int numeric, failure;


static FVM_Result convert(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    FVM_Value value;
    (void)raw;
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, frame->receiver, "alive", &value) == FVM_OK &&
          value.number == 42))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure)
    {
        fvm_value_number(&value, 77);
        return fvm_exception_raise(c, &value);
    }
    if (numeric)
    {
        fvm_value_number(out, 65537);
        return FVM_OK;
    }
    return fvm_value_string_bytes(out, "a\0b", 3);
}


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned version;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (version = 0; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Value in, out, value;
        FVM_Object *obj, *fn;
        const unsigned char code[] = {0};
        unsigned short pair[] = {0xd800, 0xdc00};
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        int legacy = version && version < 6;
        if (version && version < 4)
        {
            continue;
        }
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            swf[3] = (unsigned char)version;
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&in, "a\0b", 3);
        if (!(fvm_exec_string_length(c, &in, &out) == FVM_OK && out.number == 3 &&
              fvm_exec_char_code(c, &in, &out) == FVM_OK && out.number == 97))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&in, "");
        if (!(fvm_exec_char_code(c, &in, &out) == FVM_OK && out.number == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&in);
        if (!(fvm_exec_string_length(c, &in, &out) == FVM_OK && out.number == (version && version < 7 ? 0 : 9)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_value_string_utf16(c, pair, 2, &in) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_string_length(c, &in, &out) == (legacy ? FVM_UNSUPPORTED : FVM_OK) &&
              out.number == (legacy ? 99 : 2)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_char_code(c, &in, &out) == (legacy ? FVM_UNSUPPORTED : FVM_OK) &&
              out.number == (legacy ? 99 : 65533)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&in, 65537);
        if (!(fvm_exec_char_from_code(c, &in, &in) == FVM_OK && in.type == FVM_VALUE_STRING &&
              !strcmp(in.string, "\1")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&in, -65471);
        if (!(fvm_exec_char_from_code(c, &in, &out) == FVM_OK && !strcmp(out.string, "A")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&in, HUGE_VAL);
        if (!(fvm_exec_char_from_code(c, &in, &out) == FVM_OK && fvm_value_string_size(&out) == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&in, 55296);
        fvm_value_number(&out, 99);
        if (!(fvm_exec_char_from_code(c, &in, &out) == (legacy ? FVM_UNSUPPORTED : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (legacy)
        {
            if (!(out.number == 99))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(fvm_value_string_size(&out) == 3 && !memcmp(out.string, "\xef\xbf\xbd", 3)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_create(c, &obj) == FVM_OK && fvm_gc_pin(c, obj) == FVM_OK &&
              fvm_function_create_avm1(c, code, sizeof(code), 0, 0, 0, &fn) == FVM_OK &&
              fvm_function_set_body(c, fn, convert, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 42);
        if (!(fvm_object_set(c, obj, "alive", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = fn;
        if (!(fvm_object_set(c, obj, "toString", &value) == FVM_OK &&
              fvm_object_set(c, obj, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        in.type = FVM_VALUE_OBJECT;
        in.object = obj;
        numeric = 0;
        if (!(fvm_exec_string_length(c, &in, &out) == FVM_OK && out.number == 3 &&
              fvm_exec_char_code(c, &in, &out) == FVM_OK && out.number == 97))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        numeric = 1;
        if (!(fvm_exec_char_from_code(c, &in, &out) == FVM_OK && !strcmp(out.string, "\1")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failure = 1;
        numeric = 0;
        fvm_value_number(&out, 99);
        if (!(fvm_exec_string_length(c, &in, &out) == FVM_EXCEPTION && out.number == 99 &&
              fvm_exception_clear(c) == FVM_OK && fvm_exec_char_code(c, &in, &out) == FVM_EXCEPTION &&
              out.number == 99 && fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        numeric = 1;
        if (!(fvm_exec_char_from_code(c, &in, &out) == FVM_EXCEPTION && out.number == 99 &&
              fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        failure = 0;
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
