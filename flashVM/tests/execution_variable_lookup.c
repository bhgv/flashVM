/* C89 plain binding reads preserve undefined presence and explicit call-receiver policy. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/property.h"
#include "flashvm/swf.h"


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
        FVM_Object *scope, *target, *receiver, *fallback, *owner, *global;
        FVM_Value name, value, out;
        int found;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_create(c, &scope) == FVM_OK && fvm_object_create(c, &target) == FVM_OK &&
              fvm_object_create(c, &receiver) == FVM_OK && fvm_object_create(c, &fallback) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_object_set(c, scope, "local", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 23);
        if (!(fvm_object_set(c, target, "target", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 31);
        if (!(fvm_set_global(c, "global", &value) == FVM_OK && fvm_set_global(c, "THIS", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "local");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 1, &name, &out, &owner, &found) ==
                  FVM_OK &&
              found && out.number == 17 && owner == fallback))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "target");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 1, &name, &out, &owner, &found) ==
                  FVM_OK &&
              found && out.number == 23 && owner == target))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "global");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 1, &name, &name, &owner, &found) ==
                  FVM_OK &&
              found && name.number == 31 && owner == fallback))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(fvm_object_set(c, scope, "u", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "u");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 1, &name, &out, &owner, &found) ==
                  FVM_OK &&
              found && out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&name, "THIS");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 1, &name, &out, &owner, &found) ==
                  FVM_OK &&
              found && owner == fallback))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version == 6 ? (out.type == FVM_VALUE_OBJECT && out.object == receiver)
                           : (out.type == FVM_VALUE_NUMBER && out.number == 31)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (version == 5)
        {
            if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 0, &name, &out, &owner, &found) ==
                      FVM_OK &&
                  found && out.object == receiver))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(&name, "_global");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, 0, 1, &name, &out, &owner, &found) == FVM_OK &&
              (version >= 6 ? (found && out.type == FVM_VALUE_OBJECT && owner == out.object)
                            : (!found && out.type == FVM_VALUE_UNDEFINED))))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        global = owner;
        fvm_value_string(&name, "missing");
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, 0, 1, &name, &out, &owner, &found) == FVM_OK &&
              !found && out.type == FVM_VALUE_UNDEFINED && owner == global))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&name, 1);
        fvm_value_number(&out, 99);
        owner = scope;
        found = 99;
        if (!(fvm_exec_variable_lookup(c, scope, target, receiver, fallback, 1, &name, &out, &owner, &found) ==
                  FVM_INVALID_ARGUMENT &&
              out.number == 99 && owner == scope && found == 99))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
