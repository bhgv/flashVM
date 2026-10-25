/* A deeper inherited virtual consumes a deferred watcher exception across data owners. */
#define main deep_watch_baseline_main
#include <stdio.h>
#include <stdlib.h>
#include "object_watch_throw.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *keys[] = {"slot", "5", "+5", "n\0m"};
    const size_t sizes[] = {4, 1, 2, 3};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned array = 0; array < 2; ++array)
        {
            for (unsigned key = 0; key < 4; ++key)
            {
                for (setter_throws = 0; setter_throws < 2; ++setter_throws)
                {
                    for (unsigned getter_only = 0; getter_only < 2; ++getter_only)
                    {
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
                        FVM_Object *receiver = 0, *middle = 0, *far = 0, *watch = 0, *get = 0, *set = 0;
                        if (!(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK &&
                              fvm_gc_pin(c, receiver) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_construct(c, "Object", 0, 0, &middle) == FVM_OK &&
                              fvm_construct(c, "Object", 0, 0, &far) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        FVM_Value value, args[3], out;
                        fvm_value_number(&value, 9);
                        if (!(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_set_bytes(c, far, keys[key], sizes[key], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        const unsigned char end[] = {0};
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK &&
                              fvm_function_set_body(c, watch, throwing_watch, 0, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK &&
                              fvm_function_set_body(c, get, throw_get, 0, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &set) == FVM_OK &&
                              fvm_function_set_body(c, set, throw_set, 0, 0) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        fvm_value_string_bytes(args, keys[key], sizes[key]);
                        args[1] = thrown_object(get);
                        if (getter_only)
                        {
                            fvm_value_undefined(args + 2);
                            args[2].type = FVM_VALUE_NULL;
                        }
                        else
                        {
                            args[2] = thrown_object(set);
                        }
                        if (!(fvm_object_call(c, far, "addProperty", 3, args, &out) == FVM_OK && out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK &&
                              fvm_object_set_prototype(c, middle, far) == FVM_OK &&
                              fvm_object_set_prototype(c, receiver, middle) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        args[1] = thrown_object(watch);
                        if (!(fvm_object_call(c, receiver, "watch", 2, args, &out) == FVM_OK && out.boolean))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        setter_calls = 0;
                        fvm_value_number(&value, 81);
                        const FVM_Result result = fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value);
                        if (!(result == (!getter_only && setter_throws ? FVM_EXCEPTION : FVM_OK)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(setter_calls == (getter_only ? 0U : 1U)))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (result == FVM_EXCEPTION)
                        {
                            if (!(fvm_exception_get(c, &out) == FVM_OK && out.number == 88))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                            if (!(fvm_exception_clear(c) == FVM_OK))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(!c->exception_pending))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        if (array)
                        {
                            if (!(fvm_object_get(c, receiver, "length", &out) == FVM_OK && out.number == 0))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        int own = 1;
                        if (!(fvm_object_has_own_bytes(c, receiver, keys[key], sizes[key], &own) == FVM_OK && !own))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(fvm_object_get_bytes(c, middle, keys[key], sizes[key], &out) == FVM_OK &&
                              out.number == 9))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner &&
                              c->watch_running.empty()))
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
