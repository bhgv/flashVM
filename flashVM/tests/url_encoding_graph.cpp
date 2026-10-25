/* Stable URL Function identities survive sender teardown and keep trusted collecting coercion. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include <cstring>

static unsigned calls = 0;


static FVM_Result stringify(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_value_string(out, "a+b c");
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
    /* Existing numeric IDs must never be reinterpreted as the newly appended globals. */
    FVM_Context *probe = 0;
    if (!(fvm_context_create(rt, &probe) == FVM_OK && fvm_standard_install(probe) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *old_names[] = {"Number", "Boolean", "isNaN", "isFinite", "parseInt", "parseFloat", "String"};
    for (unsigned i = 0; i < 7; ++i)
    {
        if (!(fvm_numeric_callback_id(probe->functions[old_names[i]].callback) == 150 + i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_numeric_callback_from_id(157) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_numeric_callback_id(probe->global->accessors["NaN"].getter) == 158))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_numeric_callback_id(probe->global->accessors["Infinity"].getter) == 159))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_numeric_callback_id(probe->global->accessors["NaN"].setter) == 160))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(probe);
    for (unsigned decode = 0; decode < 2; ++decode)
    {
        FVM_Context *a = 0, *b = 0;
        if (!(fvm_context_create(rt, &a) == FVM_OK && fvm_context_create(rt, &b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_standard_install(a) == FVM_OK && fvm_standard_install(b) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value fn;
        if (!(fvm_get_global(a, decode ? "unescape" : "escape", &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char *bytes = 0;
        size_t size = 0;
        if (!(fvm_value_serialize(a, &fn, &bytes, &size) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(a);
        FVM_Value imported;
        if (!(fvm_value_deserialize(b, bytes, size, &imported) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_serialized_free(bytes);
        if (!(imported.type == FVM_VALUE_OBJECT && imported.object->invocable.collector_entry))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_pin(b, imported.object) == FVM_OK && fvm_gc_collect(b, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *o = 0;
        if (!(fvm_object_create(b, &o) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(b, o, "toString", stringify, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        o->methods["toString"].collector_entry = true;
        FVM_Value arg, out;
        fvm_value_undefined(&arg);
        arg.type = FVM_VALUE_OBJECT;
        arg.object = o;
        unsigned before = calls;
        if (!(fvm_function_invoke(b, imported.object, 0, 1, &arg, &out) == FVM_OK && calls == before + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const char *expected = decode ? "a b c" : "a%2Bb%20c";
        if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == std::strlen(expected) &&
              !std::memcmp(out.string, expected, std::strlen(expected))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!b->gc_frames && !b->gc_native_depth && !b->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(b);
    }
    fvm_runtime_destroy(rt);
}
