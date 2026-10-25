/* Intrinsic namespaces are visible to member/path/delete services without becoming own slots. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/execution.h"
#include "flashvm/standard.h"
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
    for (version = 0; version <= 8; ++version)
    {
        FVM_Context *c;
        FVM_Object *clip, *proto, *global, *owner, *module, *derived;
        FVM_Value key, value, out, clip_value;
        int found, handled, present, own, removed;
        unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
        int intrinsic = version >= 6, alias = !version || version >= 6;
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
        if (!(fvm_standard_install(c) == FVM_OK && fvm_movieclip_create(c, "root", 1, &clip) == FVM_OK &&
              fvm_object_create(c, &proto) == FVM_OK && fvm_object_set_prototype(c, clip, proto) == FVM_OK &&
              fvm_object_create(c, &module) == FVM_OK && fvm_object_create(c, &derived) == FVM_OK &&
              fvm_object_set_prototype(c, derived, clip) == FVM_OK))
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
        fvm_value_undefined(&clip_value);
        clip_value.type = FVM_VALUE_OBJECT;
        clip_value.object = clip;
        if (!(fvm_set_global(c, "clip", &clip_value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 17);
        if (!(fvm_set_global(c, "marker", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global");
        if (!(fvm_exec_member(c, 0, &clip_value, &key, 0, &out) == FVM_OK &&
              out.type == (intrinsic ? FVM_VALUE_OBJECT : FVM_VALUE_UNDEFINED)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (intrinsic)
        {
            if (!(out.object == global))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_has_own(c, clip, "_global", &own) == FVM_OK && !own))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "clip:_global");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &key, &out, &handled, &present) == FVM_OK && handled &&
              present == intrinsic && out.type == (intrinsic ? FVM_VALUE_OBJECT : FVM_VALUE_UNDEFINED)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global:marker");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &key, &out, &handled, &present) == FVM_OK && handled &&
              present == alias))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (alias)
        {
            if (!(out.number == 17))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_number(&value, 41);
        if (!(fvm_object_set(c, proto, "_global", &value) == FVM_OK &&
              fvm_object_get(c, clip, "_global", &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(intrinsic ? (out.type == FVM_VALUE_OBJECT && out.object == global)
                        : (out.type == FVM_VALUE_NUMBER && out.number == 41)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (intrinsic)
        {
            fvm_value_string(&key, "_global");
            if (!(fvm_exec_variable_lookup(c, 0, derived, 0, 0, 0, &key, &out, &owner, &found) == FVM_OK && found &&
                  out.object == global && fvm_object_get(c, derived, "_global", &out) == FVM_OK &&
                  out.object == global))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        fvm_value_undefined(&value);
        if (!(fvm_object_set(c, clip, "_global", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "clip:_global");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &key, &out, &handled, &present) == FVM_OK && handled && present &&
              out.type == FVM_VALUE_UNDEFINED && fvm_object_has_own(c, clip, "_global", &own) == FVM_OK && own))
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
        fvm_value_number(&value, 31);
        if (!(fvm_object_set(c, module, "marker", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = module;
        if (!(fvm_set_global(c, "_global", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global:marker");
        if (!(fvm_exec_variable_path(c, 0, 0, 0, &key, &out, &handled, &present) == FVM_OK && handled && present &&
              out.number == (version ? 31 : 17)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_exec_variable_path(c, 0, clip, 0, &key, &out, &handled, &present) == FVM_OK && handled && present &&
              out.number == (intrinsic || !version ? 17 : 31)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_delete(c, global, "_global", &removed) == FVM_OK && removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_string(&key, "_global:marker");
        if (!(fvm_exec_delete_reference(c, 0, clip, 0, &key, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN &&
              out.boolean == alias))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_has_own(c, global, "marker", &own) == FVM_OK && own == !alias))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
