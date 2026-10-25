/* Error message setters observe prepared identities and preserve exceptions. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/property.h"

static FVM_Object *error_class;
static FVM_Object *error_message;
static FVM_Object *error_receiver;
static FVM_Result error_status;
static unsigned error_calls;


static FVM_Result
message_setter(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *args, FVM_Value *out, void *)
{
    ++error_calls;
    error_receiver = self;
    if (!(count == 1 && args[0].object == error_message))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    if (!(fvm_object_get(c, self, "__constructor__", &value) == FVM_OK && value.object == error_class))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    int own = 0;
    if (!(fvm_object_has_own(c, self, "constructor", &own) == FVM_OK && !!own == (c->swf_info.version < 7)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Force publication of this otherwise private receiver from the callback. */
    value = class_value(self);
    if (!(fvm_set_global(c, "observedError", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    *out = args[0];
    return error_status == FVM_EXCEPTION ? fvm_exception_raise(c, out) : error_status;
}


int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 4; version <= 8; ++version)
    {
        for (unsigned route = 0; route < 8; ++route)
        {
            for (unsigned failure = 0; failure < 3; ++failure)
            {
                if (route >= 2 && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = 0;
                if (!(fvm_context_create(runtime, &c) == FVM_OK))
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
                FVM_Value klass, prototype, input, out;
                if (!(fvm_get_global(c, "Error", &klass) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                error_class = klass.object;
                if (!(fvm_object_get(c, error_class, "prototype", &prototype) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                int removed = 0;
                if (!(fvm_object_set_property_flags(c, prototype.object, "message", 0, FVM_PROPERTY_VALID_FLAGS) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_delete(c, prototype.object, "message", &removed) == FVM_OK && removed))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_define_accessor_rooted(c, prototype.object, "message", 0, message_setter, 0, 0) ==
                      FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_object_create(c, &error_message) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                input = class_value(error_message);
                if (!(fvm_object_set(c, error_message, "cycle", &input) == FVM_OK &&
                      fvm_set_global(c, "messageInput", &input) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                FVM_Object *owner = 0;
                if (!(fvm_object_create(c, &owner) == FVM_OK && fvm_object_set(c, owner, "Error", &klass) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                out = class_value(owner);
                if (!(fvm_set_global(c, "constructors", &out) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                error_status = failure == 0 ? FVM_OK : failure == 1 ? FVM_ERROR : FVM_EXCEPTION;
                error_receiver = 0;
                error_calls = 0;
                FVM_Object *instance = (FVM_Object *)1;
                if (route < 2)
                {
                    FVM_Result status = route ? fvm_construct(c, "Error", 1, &input, &instance)
                                              : fvm_function_construct(c, error_class, 1, &input, &instance);
                    if (!(status == error_status && (status == FVM_OK ? instance == error_receiver : !instance)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                    if (status == FVM_EXCEPTION)
                    {
                        if (!(c->exception_pending && c->exception_value.object == error_message))
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
                }
                else
                {
                    class_mode = route >= 5 ? route - 5 : route - 2;
                    Program code;
                    code.variable("messageInput");
                    code.number(1);
                    if (route >= 5)
                    {
                        code.variable("constructors");
                        code.text("Error");
                        code.op(0x53);
                    }
                    else
                    {
                        code.text("Error");
                        code.op(0x40);
                    }
                    code.finish();
                    out = class_run(c, code, error_status);
                    if (error_status == FVM_OK)
                    {
                        if (!(out.object == error_receiver))
                        {
                            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                            exit(EXIT_FAILURE);
                        }
                    }
                }
                if (!(error_calls == 1 && error_receiver && error_receiver->prototype == prototype.object))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_get_global(c, "observedError", &out) == FVM_OK && out.object == error_receiver))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
