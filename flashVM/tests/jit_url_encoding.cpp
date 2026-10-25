/* Actual SWF5--8 global/extracted URL calls and collecting ToString scripts in all execution modes. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include <cstring>

static unsigned collections = 0;


static FVM_Result collect(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++collections;
    fvm_value_undefined(out);
    return FVM_OK;
}


static void same(const FVM_Value &v, const char *s)
{
    if (!(v.type == FVM_VALUE_STRING && fvm_value_string_size(&v) == std::strlen(s) &&
          !std::memcmp(v.string, s, std::strlen(s))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static FVM_Value url_run(FVM_Context *c, const Program &p, FVM_Result expected = FVM_OK)
{
    FVM_Object *fn = 0;
    FVM_JitCode *code = 0;
    FVM_Value out;
    if (!(fvm_function_create_avm1(c, p.bytes.begin(), p.bytes.size(), 0, 0, 0, &fn) == FVM_OK &&
          fvm_gc_pin(c, fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode == 1)
    {
        if (!(fvm_jit_compile(p.bytes.begin(), p.bytes.size(), &code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_execution_set_mode(c, class_mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned long entries = c->execution_mode.stats.native_entries;
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (class_mode)
    {
        if (!(c->execution_mode.stats.native_entries > entries))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(c->execution_mode.stats.native_entries == entries))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->script_depth && (!code || !code->active)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected == FVM_EXCEPTION)
    {
        if (!(fvm_exception_clear(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (code)
    {
        if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_destroy(code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_gc_unpin(c, fn) == FVM_OK))
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
    for (unsigned version = 5; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < (fvm_jit_available() ? 3U : 1U); ++class_mode)
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
            for (unsigned decode = 0; decode < 2; ++decode)
            {
                const char *name = decode ? "unescape" : "escape";
                Program direct;
                direct.text(decode ? "a%40b%2Ec" : "a@b.c");
                direct.number(1);
                direct.text(name);
                direct.op(0x3d);
                direct.finish();
                same(url_run(c, direct), decode ? "a@b.c" : "a%40b%2Ec");
                FVM_Value fn;
                if (!(fvm_get_global(c, name, &fn) == FVM_OK && fvm_set_global(c, "codecAlias", &fn) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                Program alias;
                alias.text(decode ? "x%2By+z" : "x+y z");
                alias.number(1);
                alias.text("codecAlias");
                alias.op(0x3d);
                alias.finish();
                same(url_run(c, alias), decode ? "x+y z" : "x%2By%20z");
            }
            FVM_Object *o = 0;
            if (!(fvm_object_create(c, &o) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_Value v = class_value(o);
            if (!(fvm_set_global(c, "source", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_register_function(c, "collectUrl", collect, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            c->functions["collectUrl"].collector_entry = true;
            Program stringify;
            stringify.number(0);
            stringify.text("collectUrl");
            stringify.op(0x3d);
            stringify.op(0x17);
            stringify.text("a@b.c");
            stringify.op(0x3e);
            stringify.op(0);
            FVM_Object *fn = 0;
            if (!(fvm_function_create_avm1(c, stringify.bytes.begin(), stringify.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v = class_value(fn);
            if (!(fvm_object_set(c, o, "toString", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program conversion;
            conversion.variable("source");
            conversion.number(1);
            conversion.text("escape");
            conversion.op(0x3d);
            conversion.finish();
            unsigned before = collections;
            same(url_run(c, conversion), "a%40b%2Ec");
            if (!(collections == before + 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program bad;
            bad.text("%GG");
            bad.number(1);
            bad.text("unescape");
            bad.op(0x3d);
            bad.finish();
            url_run(c, bad, FVM_UNSUPPORTED);
            Program throwing;
            throwing.text("codec coercion");
            throwing.op(0x2a);
            throwing.op(0);
            if (!(fvm_function_create_avm1(c, throwing.bytes.begin(), throwing.bytes.size(), 0, 0, 0, &fn) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            v = class_value(fn);
            if (!(fvm_object_set(c, o, "toString", &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            url_run(c, conversion, FVM_EXCEPTION);
            if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth && !c->script_depth &&
                  !c->string_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
}
