/* Namespace fallback obeys SWF6 availability and follows ordinary bindings, including undefined. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/property.h"


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
        FVM_Object *target, *scope, *owner, *global;
        FVM_Value key, value, out;
        int found, removed;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        int available = !version || version >= 6;
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
        if (!(fvm_object_create(c, &target) == FVM_OK && fvm_object_create(c, &scope) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global");
        if (!(fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK &&
              found == available && out.type == (available ? FVM_VALUE_OBJECT : FVM_VALUE_UNDEFINED)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        global = owner;
        if (available)
        {
            if (!(out.object == global))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_string(&key, "_GLOBAL");
        if (!(fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK &&
              found == (version == 6)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!version)
        {
            fvm_value_number(&value, 31);
            if (!(fvm_set_global(c, "_global", &value) == FVM_OK &&
                  fvm_object_set(c, target, "_global", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_string(&key, "_global");
            if (!(fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
                  out.type == FVM_VALUE_OBJECT && out.object == global))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_value_number(&value, 55);
            if (!(fvm_object_set(c, scope, "_global", &value) == FVM_OK &&
                  fvm_exec_variable_lookup(c, scope, target, 0, 0, 1, &key, &out, &owner, &found) == FVM_OK && found &&
                  out.number == 55))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
            continue;
        }
        fvm_value_number(&value, 31);
        if (!(fvm_set_global(c, "_global", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global");
        if (!(fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              out.number == 31))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 41);
        if (!(fvm_object_set(c, target, "_global", &value) == FVM_OK &&
              fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              out.number == 41 && owner == target))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(fvm_object_set(c, target, "_global", &value) == FVM_OK &&
              fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              out.type == FVM_VALUE_UNDEFINED && owner == target))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, target, "_global", &removed) == FVM_OK && removed &&
              fvm_set_global(c, "_global", &value) == FVM_OK &&
              fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, global, "_global", &removed) == FVM_OK && removed &&
              fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK &&
              found == available))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 55);
        if (!(fvm_object_set(c, scope, "_global", &value) == FVM_OK &&
              fvm_exec_variable_lookup(c, scope, target, 0, 0, 1, &key, &out, &owner, &found) == FVM_OK && found &&
              out.number == 55))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string_bytes(&key, "_global\0x", 9);
        if (!(fvm_exec_variable_lookup(c, 0, target, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && !found &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
