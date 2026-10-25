/* Watch converts the name before selecting a boxing constructor for its callback. */
#include <stdio.h>
#include <stdlib.h>
#include "watch_boxing_fixture.hpp"


struct WatchNameFixture
{
    WatchBoxingFixture *box;
    const char *name;
    unsigned fault, reads;
};


static FVM_Result
watch_name_select(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    WatchNameFixture *f = (WatchNameFixture *)raw;
    ++f->reads;
    if (!(!f->box->calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (f->fault == 1)
    {
        return FVM_ERROR;
    }
    if (f->fault == 2)
    {
        *out = class_value(f->box->target);
        return fvm_exception_raise(c, out);
    }
    FVM_Value value = class_value(f->box->factory);
    if (!(fvm_set_global(c, f->name, &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fvm_value_string_bytes(out, "watched\0x", 9);
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
    const char *names[] = {"Number", "Boolean", "String"};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned type = 0; type < 3; ++type)
        {
            for (unsigned fault = 0; fault < 3; ++fault)
            {
                for (unsigned route = 0; route < 4; ++route)
                {
                    if (route >= 2 && !fvm_jit_available())
                    {
                        continue;
                    }
                    WatchBoxingFixture f;
                    watch_box_init(rt, f, version, type, 0);
                    FVM_Context *c = f.context;
                    WatchNameFixture name;
                    name.box = &f;
                    name.name = names[type];
                    name.fault = fault;
                    name.reads = 0;
                    FVM_Object *key = 0;
                    if (!(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK &&
                          fvm_object_define_method_rooted(c, key, "toString", watch_name_select, &name, f.factory) ==
                              FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Value args[3], missing, out;
                    args[0] = class_value(key);
                    args[1] = f.input;
                    fvm_value_number(args + 2, 7);
                    fvm_value_undefined(&missing);
                    if (!(fvm_set_global(c, names[type], &missing) == FVM_OK &&
                          fvm_set_global(c, "watchKey", args) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Result expected = fault == 1 ? FVM_ERROR : fault == 2 ? FVM_EXCEPTION : FVM_OK;
                    if (!route)
                    {
                        if (!(fvm_object_call(c, f.target, "watch", 3, args, &out) == expected))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        Program code;
                        code.number(7);
                        code.variable("watchInput");
                        code.variable("watchKey");
                        code.number(3);
                        code.variable("watchTarget");
                        code.text("watch");
                        code.op(0x52);
                        code.finish();
                        out = boxing_run(c, code, route - 1, expected);
                    }
                    if (!(name.reads == 1 && f.calls == (fault ? 0U : 1U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!fault)
                    {
                        if (!(out.type == FVM_VALUE_BOOLEAN && out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value input;
                        fvm_value_number(&input, 5);
                        if (!(fvm_object_set_bytes(c, f.target, "watched\0x", 9, &input) == FVM_OK &&
                              fvm_object_get_bytes(c, f.target, "watched\0x", 9, &out) == FVM_OK && out.number == 12))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        if (!(f.target->watchers.empty()))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (fault == 2 && !route)
                    {
                        if (!(c->exception_pending && c->exception_value.object == f.target &&
                              fvm_exception_clear(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->exception_pending &&
                          fvm_gc_collect(c, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_context_destroy(c);
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
