/* C89 versioned path phase preserves plain-name fallback and distinguishes undefined presence. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/swf.h"
#include <string.h>


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
    for (version = 4; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *o, *child;
        FVM_Value value, name, out;
        int handled, present;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &o) == FVM_OK && fvm_object_create(c, &child) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = o;
        if (!(fvm_set_global(c, "o", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = child;
        if (!(fvm_object_set(c, o, "child", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, o, "member", &value) == FVM_OK &&
              fvm_object_set_bytes(c, o, "a\0b", 3, &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(fvm_object_set(c, o, "u", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "o.member");
        fvm_value_number(&out, 99);
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version >= 5 ? (handled && present && out.number == 17) : (!handled && !present && out.number == 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "o:member");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &name, &handled, &present) == FVM_OK && handled && present &&
              name.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "o:u");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK && handled && present &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "o:missing");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK && handled && !present &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, ":member");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK && handled && !present &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&name, "o:a\0b", 5);
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK && handled && present &&
              out.number == 17))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "o/child");
        fvm_value_number(&out, 99);
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version >= 5 ? (handled && present && out.object == child) : (!handled && out.number == 99)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "o/missing");
        fvm_value_number(&out, 99);
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK && !handled && !present &&
              out.number == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "plain");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &name, &handled, &present) == FVM_OK && !handled &&
              name.type == FVM_VALUE_STRING && !strcmp(name.string, "plain")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version >= 5)
        {
            fvm_value_string(&name, "_global.o");
            if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_OK && handled &&
                  (version >= 6 ? (present && out.object == o) : (!present && out.type == FVM_VALUE_UNDEFINED))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&name, "this.member");
            if (!(fvm_exec_variable_path(c, 0, 0, o, &name, &out, &handled, &present) == FVM_OK && handled && present &&
                  out.number == 17))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&name, 1);
        fvm_value_number(&out, 99);
        handled = 21;
        present = 22;
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &name, &out, &handled, &present) == FVM_INVALID_ARGUMENT &&
              out.number == 99 && handled == 21 && present == 22))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
