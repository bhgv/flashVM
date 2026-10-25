/* Compare native named calls/construction with interpretation, including collecting name/count coercion. */
#define FVM_METHOD_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_methods.cpp"


static FVM_Result name_value(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    FVM_Result status = fvm_gc_collect(c, 0);
    return status == FVM_OK ? fvm_value_string(out, "run") : status;
}


static FVM_Result count_value(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    FVM_Result status = fvm_gc_collect(c, 0);
    fvm_value_number(out, 2);
    return status;
}


static FVM_Result marker(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *data)
{
    FVM_Value value;
    fvm_value_number(&value, *(const double *)data);
    fvm_value_undefined(out);
    return fvm_object_set(c, frame->receiver, "marker", &value);
}
#ifndef FVM_NAMED_FIXTURE_ONLY


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
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
        FVM_Object *owner = 0, *method = 0, *conversion = 0;
        FVM_Value value;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version)
        {
            if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &owner) == FVM_OK && fvm_gc_pin(c, owner) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned char empty[] = {0};
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &method) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, method, called, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = method;
        if (!(fvm_object_set(c, c->global, "run", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &conversion) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, conversion, name_value, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = conversion;
        if (!(fvm_object_set(c, owner, "toString", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &conversion) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, conversion, count_value, 0, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = conversion;
        if (!(fvm_object_set(c, owner, "valueOf", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned construct = 0; construct < 2; ++construct)
        {
            Program p;
            p.number(25);
            p.number(17);
            p.number(2);
            p.text("run");
            p.op(construct ? 0x40 : 0x3d);
            p.finish();
            throwing = false;
            check(c, p, owner, construct ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER, 42, FVM_OK, true);
            throwing = true;
            check(c, p, owner, FVM_VALUE_UNDEFINED, 0, FVM_EXCEPTION, true);
            throwing = false;
            Program local;
            local.text("localRun");
            local.text("run");
            local.op(0x1c);
            local.op(0x3c);
            local.number(25);
            local.number(17);
            local.number(2);
            local.text("localRun");
            local.op(construct ? 0x40 : 0x3d);
            local.finish();
            check(c, local, owner, construct ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER, 42, FVM_OK, true);
            Program name;
            name.number(25);
            name.number(17);
            name.number(2);
            name.variable("input");
            name.op(construct ? 0x40 : 0x3d);
            name.finish();
            check(c,
                  name,
                  owner,
                  construct ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER,
                  42,
                  version ? FVM_OK : FVM_ERROR,
                  version != 0);
            Program count;
            count.number(25);
            count.number(17);
            count.variable("input");
            count.text("run");
            count.op(construct ? 0x40 : 0x3d);
            count.finish();
            check(c,
                  count,
                  owner,
                  construct ? FVM_VALUE_OBJECT : FVM_VALUE_NUMBER,
                  42,
                  version ? FVM_OK : FVM_ERROR,
                  version != 0);
        }
        /* Loaded NewObject treats dotted names literally; host construction follows paths. */
        const double path_value = 99, literal_value = 77;
        FVM_Object *space = 0, *path_ctor = 0, *literal_ctor = 0;
        if (!(fvm_object_create(c, &space) == FVM_OK && fvm_gc_pin(c, space) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &path_ctor) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, path_ctor, marker, (void *)&path_value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = path_ctor;
        if (!(fvm_object_set(c, space, "Ctor", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &literal_ctor) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, literal_ctor, marker, (void *)&literal_value, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = literal_ctor;
        if (!(fvm_set_global(c, "space.Ctor", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = space;
        if (!(fvm_set_global(c, "space", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        Program dotted;
        dotted.number(0);
        dotted.text("space.Ctor");
        dotted.op(0x40);
        dotted.text("marker");
        dotted.op(0x4e);
        dotted.finish();
        check(c, dotted, owner, FVM_VALUE_NUMBER, version ? 77 : 99);
        Program colon;
        colon.number(0);
        colon.text("space:Ctor");
        colon.op(0x40);
        colon.text("marker");
        colon.op(0x4e);
        colon.finish();
        check(c, colon, owner, version ? FVM_VALUE_UNDEFINED : FVM_VALUE_NUMBER, 99);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
