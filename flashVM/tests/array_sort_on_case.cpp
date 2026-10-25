/* Loaded sortOn resolves own raw fields with versioned case, preserving stored virtual data. */
#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"

static unsigned sort_field_calls;


static FVM_Result sort_field_trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++sort_field_calls;
    fvm_value_number(out, 999);
    return FVM_OK;
}
#ifdef FVM_SORT_FIELD_CASE_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"

#endif


static void sort_case_call(FVM_Context *c, FVM_Object *array, FVM_Value *args, FVM_Value *out)
{
#ifdef FVM_SORT_FIELD_CASE_JIT
    FVM_Value value = object(array);
    if (!(fvm_set_global(c, "sortCaseArray", &value) == FVM_OK && fvm_set_global(c, "sortCaseField", args) == FVM_OK &&
          fvm_set_global(c, "sortCaseFlags", args + 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Program call;
    call.variable("sortCaseFlags");
    call.variable("sortCaseField");
    call.number(2);
    call.variable("sortCaseArray");
    call.text("sortOn");
    call.op(0x52);
    call.finish();
    *out = class_run(c, call);
#else
    if (!(fvm_object_call(c, array, "sortOn", 2, args, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
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
#ifdef FVM_SORT_FIELD_CASE_JIT
    if (!fvm_jit_available())
    {
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
        for (unsigned version = 4; version <= 8; ++version)
        {
            for (unsigned kind = 0; kind < 5; ++kind)
            {
                for (unsigned name_kind = 0; name_kind < 2; ++name_kind)
                {
                    for (unsigned policy = 0; policy < 3; ++policy)
                    {
                        if (kind == 3 && name_kind)
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
                        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const char *stored = kind == 3 ? "length" : name_kind ? "\xc3\x84Name" : "MiXeD\0Name";
                        const char *requested = kind == 3 ? "LENGTH" : name_kind ? "\xc3\xa4name" : "mixed\0name";
                        size_t size = kind == 3 ? 6 : name_kind ? 6 : 10;
                        FVM_Object *rows[2], *fn = 0, *array = 0;
                        FVM_Value values[2], value, args[3], out;
                        if (!(fvm_function_create_native(c, sort_field_trap, 0, 0, &fn) == FVM_OK &&
                              fvm_gc_pin(c, fn) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        for (unsigned i = 0; i < 2; ++i)
                        {
                            fvm_value_number(&value, i ? 2 : 10);
                            if (!(fvm_construct(c,
                                                kind == 3 ? "Array" : "Object",
                                                kind == 3 ? 1 : 0,
                                                kind == 3 ? &value : 0,
                                                rows + i) == FVM_OK &&
                                  fvm_gc_pin(c, rows[i]) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            FVM_Object *owner = rows[i];
                            if (kind == 2)
                            {
                                if (!(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK &&
                                      fvm_object_set_prototype(c, rows[i], owner) == FVM_OK))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            if (kind != 3)
                            {
                                if (kind == 4 && i)
                                {
                                    if (!(fvm_object_define_method_rooted_bytes(
                                              c, owner, stored, size, sort_field_trap, 0, 0) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                                else
                                {
                                    if (kind == 4)
                                    {
                                        fvm_value_string(&value, "abc");
                                    }
                                    if (!(fvm_object_set_bytes(c, owner, stored, size, &value) == FVM_OK))
                                    {
                                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                        exit(EXIT_FAILURE);
                                    }
                                }
                            }
                            if (kind == 1)
                            {
                                fvm_value_string_bytes(args, stored, size);
                                args[1] = args[2] = object(fn);
                                if (!(fvm_object_call(c, owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                            values[i] = object(rows[i]);
                        }
                        if (!(fvm_construct(c, "Array", 2, values, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_string_bytes(args, requested, size);
                        fvm_value_number(args + 1, (kind == 4 ? 0 : 16) + (policy == 1 ? 8 : policy == 2 ? 4 : 0));
                        sort_field_calls = 0;
                        sort_case_call(c, array, args, &out);
                        if (!(!sort_field_calls))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        bool found = version <= 6 && kind != 2;
                        if (policy == 2 && !found)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 0))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else if (policy == 1)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT && out.object != array))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK &&
                                  value.number == (found ? 1 : 0)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_table_get_index(c, array, 0, &value) == FVM_OK && value.object == rows[0]))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.type == FVM_VALUE_OBJECT && out.object == array))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_table_get_index(c, array, 0, &value) == FVM_OK &&
                                  value.object == rows[found ? 1 : 0]))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (kind == 4)
                        {
                            if (!(rows[1]->method_values.size() == (found ? 1U : 0U)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (found)
                            {
                                fvm_detail::map<fvm_detail::string, FVM_Object *>::iterator cached =
                                    rows[1]->method_values.find(fvm_detail::string(stored, size));
                                if (!(cached != rows[1]->method_values.end()))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                                if (!(fvm_object_get_bytes(c, rows[1], requested, size, &value) == FVM_OK &&
                                      value.object == cached->second))
                                {
                                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
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
