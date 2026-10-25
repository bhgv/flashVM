/* C89 literal construction preserves order, own Array slots, counted keys and output aliases. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c, *other;
        FVM_Object *foreign;
        FVM_Value a[3], out, v;
        FVM_ExecutionLiteralEntry entries[4];
        int own;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (profile)
        {
            swf[3] = (unsigned char)(profile + 3);
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
        fvm_value_number(a, -7);
        fvm_value_undefined(a + 1);
        fvm_value_string(a + 2, "last");
        if (!(fvm_exec_literal_array(c, 3, a, a) == FVM_OK && a[0].type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, a[0].object, "length", &v) == FVM_OK && v.number == 3 &&
              fvm_object_get(c, a[0].object, "0", &v) == FVM_OK && v.number == -7))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_has_own_bytes(c, a[0].object, "1", 1, &own) == FVM_OK && own &&
              fvm_object_get(c, a[0].object, "1", &v) == FVM_OK && v.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&entries[0].name, "same");
        fvm_value_number(&entries[0].value, 1);
        entries[1] = entries[0];
        fvm_value_number(&entries[1].value, 2);
        fvm_value_string(&entries[2].name, "");
        fvm_value_number(&entries[2].value, 3);
        if (!(fvm_value_string_bytes(&entries[3].name, "a\0b", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&entries[3].value, 4);
        if (!(fvm_exec_literal_object(c, 4, entries, &entries[0].value) == FVM_OK &&
              entries[0].value.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_get(c, entries[0].value.object, "same", &v) == FVM_OK && v.number == 2 &&
              fvm_object_get_bytes(c, entries[0].value.object, "a\0b", 3, &v) == FVM_OK && v.number == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_has_own_bytes(c, entries[0].value.object, "", 0, &own) == FVM_OK && own == !profile))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_literal_array(c, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT &&
              fvm_exec_literal_object(c, 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&out, 99);
        if (!(fvm_exec_literal_array(c, 1, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99 &&
              fvm_exec_literal_object(c, 1, 0, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_literal_array(c, FVM_EXEC_MAX_LITERAL_ENTRIES + 1U, a, &out) == FVM_INVALID_ARGUMENT &&
              out.number == 99 &&
              fvm_exec_literal_object(c, FVM_EXEC_MAX_LITERAL_ENTRIES + 1U, entries, &out) == FVM_INVALID_ARGUMENT &&
              out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(other, &foreign) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = foreign;
        if (!(fvm_exec_literal_array(c, 1, &v, &out) == FVM_INVALID_ARGUMENT && out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        entries[0].name = v;
        if (!(fvm_exec_literal_object(c, 1, entries, &out) == FVM_INVALID_ARGUMENT && out.number == 99 &&
              fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(other);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
