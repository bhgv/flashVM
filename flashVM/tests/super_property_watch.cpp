/* Super watcher mutations, suspended exceptions and detached parent roots remain ordered. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "super_property_write.cpp"
#include "flashvm/exception.h"

static unsigned super_watch_mode, super_watch_calls, super_watch_setters;
static FVM_Object *super_watch_error, *super_watch_getter, *super_watch_setter;
static const char *super_watch_key;
static size_t super_watch_size;


static FVM_Result super_watch_callback(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++super_watch_calls;
    if (!(frame->argc == 4 && frame->argv[2].number == 81))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_value_string_size(frame->argv) == super_watch_size &&
          !std::memcmp(frame->argv[0].string, super_watch_key, super_watch_size)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (super_watch_mode == 2)
    {
        super_write_property(
            c, frame->receiver, super_watch_key, super_watch_size, super_watch_getter, super_watch_setter);
    }
    if (super_watch_mode == 3)
    {
        int removed = 0;
        if (!(fvm_object_delete_bytes(c, frame->receiver, super_watch_key, super_watch_size, &removed) == FVM_OK &&
              removed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (super_watch_mode == 4)
    {
        if (!(fvm_object_set_prototype(c, frame->receiver, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (super_watch_mode == 1 || super_watch_mode >= 5)
    {
        if (!(fvm_object_create(c, &super_watch_error) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Value value = class_value(super_watch_error);
        if (!(fvm_object_set(c, super_watch_error, "self", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(&value, 77);
        if (!(fvm_object_set(c, super_watch_error, "marker", &value) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        value = class_value(super_watch_error);
        return fvm_exception_raise(c, &value);
    }
    *out = frame->argv[2];
    return FVM_OK;
}


static FVM_Result super_watch_virtual(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++super_watch_setters;
    if (!(frame->receiver == super_write_receiver && frame->argc == 1 && !c->exception_pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const bool thrown = super_watch_mode == 1 || super_watch_mode >= 5;
    if (!(frame->argv[0].type == (thrown ? FVM_VALUE_UNDEFINED : FVM_VALUE_NUMBER)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (thrown)
    {
        bool alive = false;
        for (size_t i = 0; i < c->objects.size(); ++i)
        {
            if (c->objects[i] == super_watch_error)
            {
                alive = true;
            }
        }
        if (!(alive))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (super_watch_mode == 7)
    {
        fvm_value_number(out, 88);
        return fvm_exception_raise(c, out);
    }
    fvm_value_undefined(out);
    return FVM_OK;
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
    const char *keys[] = {"slot", "5", "+5", "n\0m"};
    const size_t sizes[] = {4, 1, 2, 3};
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned array = 0; array < 2; ++array)
        {
            for (unsigned key = 0; key < 4; ++key)
            {
                for (super_watch_mode = 0; super_watch_mode < 8; ++super_watch_mode)
                {
                    FVM_Context *c = 0;
                    if (!(fvm_context_create(rt, &c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    version_load(c, version);
                    if (!(fvm_standard_install(c) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *receiver = 0, *parent = 0, *watch = 0;
                    if (!(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK &&
                          fvm_gc_pin(c, receiver) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    const unsigned char end[] = {0};
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &super_watch_getter) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_set_body(c, super_watch_getter, super_write_get, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &super_watch_setter) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_set_body(c, super_watch_setter, super_watch_virtual, 0, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    super_watch_key = keys[key];
                    super_watch_size = sizes[key];
                    super_write_property(c,
                                         parent,
                                         keys[key],
                                         sizes[key],
                                         super_watch_getter,
                                         super_watch_mode == 6 ? 0 : super_watch_setter);
                    FVM_Value value, args[2], out;
                    fvm_value_number(&value, 9);
                    if (super_watch_mode == 3 || super_watch_mode == 5)
                    {
                        if (!(fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                    if (!(fvm_object_set_prototype(c, parent, receiver->prototype) == FVM_OK &&
                          fvm_object_set_prototype(c, receiver, parent) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    FVM_Object *facade = super_write_escape(c, receiver);
                    if (!(fvm_gc_pin(c, facade) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &watch) == FVM_OK &&
                          fvm_function_set_body(c, watch, super_watch_callback, 0, receiver) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    fvm_value_string_bytes(args, keys[key], sizes[key]);
                    args[1] = class_value(watch);
                    if (!(fvm_object_call(c, receiver, "watch", 2, args, &out) == FVM_OK && out.boolean))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    super_write_receiver = receiver;
                    super_watch_calls = super_watch_setters = 0;
                    fvm_value_number(&value, 81);
                    const FVM_Result status = fvm_object_set_bytes(c, facade, keys[key], sizes[key], &value);
                    if (!(super_watch_calls == 1 &&
                          super_watch_setters ==
                              (super_watch_mode == 2 || super_watch_mode == 5 || super_watch_mode == 6 ? 0U : 1U)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (!(status == (super_watch_mode == 5 || super_watch_mode == 7 ? FVM_EXCEPTION : FVM_OK)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (status == FVM_EXCEPTION)
                    {
                        if (!(fvm_exception_get(c, &out) == FVM_OK))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                        if (super_watch_mode == 5)
                        {
                            if (!(out.type == FVM_VALUE_OBJECT && out.object == super_watch_error))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
                        }
                        else
                        {
                            if (!(out.number == 88))
                            {
                                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                                exit(EXIT_FAILURE);
                            }
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
                    if (!(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && c->watch_running.empty()))
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
