/* Concrete MovieClips supply _global before prototypes/global scopes; own slots still shadow it. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/movieclip.h"
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
    for (version = 4; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *clip, *proto, *scope, *owner, *global, *class_proto, *class_scope;
        FVM_Value key, value, out;
        int found, with_scope, removed;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        swf[3] = (unsigned char)version;
        if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK &&
              fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK &&
              fvm_object_create(c, &proto) == FVM_OK && fvm_object_set_prototype(c, clip, proto) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "missing");
        if (!(fvm_exec_variable_lookup(c, 0, 0, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && !found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        global = owner;
        fvm_value_number(&value, 31);
        if (!(fvm_set_global(c, "_global", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 41);
        if (!(fvm_object_set(c, proto, "_global", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global");
        if (!(fvm_exec_variable_lookup(c, 0, clip, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              owner == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version >= 6 ? (out.type == FVM_VALUE_OBJECT && out.object == global)
                           : (out.type == FVM_VALUE_NUMBER && out.number == 41)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = clip;
        if (!(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK &&
              fvm_exec_scope_resolve(c, scope, &key, &out, &owner, &found, &with_scope) == FVM_OK && found &&
              with_scope && owner == clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version >= 6 ? (out.type == FVM_VALUE_OBJECT && out.object == global)
                           : (out.type == FVM_VALUE_NUMBER && out.number == 41)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 55);
        if (!(fvm_object_set(c, clip, "_global", &value) == FVM_OK &&
              fvm_exec_variable_lookup(c, 0, clip, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              out.number == 55 && fvm_exec_scope_resolve(c, scope, &key, &out, &owner, &found, &with_scope) == FVM_OK &&
              found && out.number == 55))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        if (!(fvm_object_set(c, clip, "_global", &value) == FVM_OK &&
              fvm_exec_scope_resolve(c, scope, &key, &out, &owner, &found, &with_scope) == FVM_OK && found &&
              out.type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, clip, "_global", &removed) == FVM_OK && removed &&
              fvm_object_delete(c, proto, "_global", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_variable_lookup(c, 0, clip, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version >= 6 ? (out.type == FVM_VALUE_OBJECT && out.object == global)
                           : (out.type == FVM_VALUE_NUMBER && out.number == 31)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_get_global(c, "MovieClip", &value) == FVM_OK &&
              fvm_object_get(c, value.object, "prototype", &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        class_proto = out.object;
        if (!(fvm_exec_variable_lookup(c, 0, class_proto, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
              out.type == FVM_VALUE_NUMBER && out.number == 31))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value.object = class_proto;
        if (!(fvm_exec_with_scope(c, 0, &value, &class_scope) == FVM_OK &&
              fvm_exec_scope_resolve(c, class_scope, &key, &out, &owner, &found, &with_scope) == FVM_OK && !found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, global, "_global", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_GLOBAL");
        if (!(fvm_exec_variable_lookup(c, 0, clip, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(version == 6 ? (found && out.type == FVM_VALUE_OBJECT && out.object == global) : !found))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
