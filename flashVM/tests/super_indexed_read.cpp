/* Super reads select visible indexed ancestors, keeping receiver slots and alias keys separate. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "super_property_write.cpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *keys[] = {"0", "5", "2147483647", "4294967295", "+5", "05", "n\0m"};
    const size_t sizes[] = {1, 1, 10, 10, 2, 2, 3};
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (unsigned array = 0; array < 2; ++array)
        {
            for (unsigned key = 0; key < 7; ++key)
            {
                for (unsigned hidden = 0; hidden < 2; ++hidden)
                {
                    for (unsigned route = 0; route < 4; ++route)
                    {
                        if ((version && version < 4) || (route >= 2 && !fvm_jit_available()))
                        {
                            continue;
                        }
                        FVM_Context *c = 0;
                        if (!(fvm_context_create(rt, &c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (version)
                        {
                            version_load(c, version);
                        }
                        if (!(fvm_standard_install(c) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *receiver = 0, *middle = 0, *far = 0;
                        if (!(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK &&
                              fvm_gc_pin(c, receiver) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_construct(c, array ? "Array" : "Object", 0, 0, &middle) == FVM_OK &&
                              fvm_construct(c, "Object", 0, 0, &far) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value value, out;
                        fvm_value_number(&value, 91);
                        if (!(fvm_object_set_bytes(c, far, keys[key], sizes[key], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_string_bytes(&value, "a\0b", 3);
                        if (!(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_number(&value, 777);
                        if (!(fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (hidden && version)
                        {
                            middle->attributes[fvm_detail::string(keys[key], sizes[key])] = version_masks[version - 4];
                        }
                        if (!(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK &&
                              fvm_object_set_prototype(c, middle, far) == FVM_OK &&
                              fvm_object_set_prototype(c, receiver, middle) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Object *facade = super_write_escape(c, receiver);
                        value = class_value(facade);
                        if (!(fvm_set_global(c, "superReadTarget", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_string_bytes(&value, keys[key], sizes[key]);
                        if (!(fvm_set_global(c, "superReadKey", &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!route)
                        {
                            if (!(fvm_object_get_bytes(c, facade, keys[key], sizes[key], &out) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            Program code;
                            code.variable("superReadTarget");
                            code.variable("superReadKey");
                            code.op(0x4e);
                            code.finish();
                            out = boxing_run(c, code, route - 1, FVM_OK);
                        }
                        if (hidden && version)
                        {
                            if (!(out.type == FVM_VALUE_NUMBER && out.number == 91))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 &&
                                  !std::memcmp(out.string, "a\0b", 3)))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (!(fvm_object_get_bytes(c, receiver, keys[key], sizes[key], &out) == FVM_OK &&
                              out.number == 777))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_owner))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_context_destroy(c);
                    }
                }
            }
        }
    }
    fvm_runtime_destroy(rt);
}
