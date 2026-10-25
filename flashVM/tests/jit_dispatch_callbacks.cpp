/* Native host/frame dispatch retains ordered coercions and original requests through nested GC. */
#define FVM_FRAME_DISPATCH_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_frame_dispatch.cpp"

static unsigned dispatch_steps, dispatch_failure, dispatch_collections;
static FVM_Object *dispatch_original_request;


static FVM_Result dispatch_convert(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *raw)
{
    unsigned id = *(unsigned *)raw;
    ++dispatch_steps;
    if (!(dispatch_steps == (id == 3 ? 1 : id)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value value;
    fvm_value_undefined(&value);
    if (!(fvm_set_global(c, id == 1 ? "target" : id == 2 ? "url" : "request", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Result status = fvm_gc_collect(c, 0);
    if (status != FVM_OK)
    {
        return status;
    }
    if (!(fvm_object_get(c, frame->receiver, "alive", &value) == FVM_OK && value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (id == 3)
    {
        dispatch_original_request = frame->receiver;
        fvm_value_string(out, "second");
    }
    else
    {
        if (!(fvm_value_string_bytes(out, id == 1 ? "t\0y" : "u\0x", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return dispatch_failure == id ? fvm_exception_raise(c, out) : FVM_OK;
}


static FVM_Result dispatch_collect(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++dispatch_collections;
    FVM_Result status = fvm_gc_collect(c, 0);
    if (status != FVM_OK)
    {
        return status;
    }
    FVM_Value value;
    if (!(fvm_object_get(c, dispatch_original_request, "alive", &value) == FVM_OK && value.number == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(out, 17);
    return FVM_OK;
}


static FVM_Object *dispatch_body(FVM_Context *c, FVM_ScriptBody body, void *raw)
{
    const unsigned char empty[] = {0};
    FVM_Object *fn = 0;
    if (!(fvm_function_create_avm1(c, empty, sizeof(empty), 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_set_body(c, fn, body, raw, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return fn;
}


static void dispatch_coercion(FVM_Context *c, unsigned *id)
{
    FVM_Object *object = 0;
    FVM_Value value;
    if (!(fvm_object_create(c, &object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_number(&value, 17);
    if (!(fvm_object_set(c, object, "alive", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(dispatch_body(c, dispatch_convert, id));
    if (!(fvm_object_set(c, object, "toString", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    value = class_value(object);
    if (!(fvm_set_global(c, *id == 1 ? "target" : *id == 2 ? "url" : "request", &value) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            if (version && version < 4)
            {
                continue;
            }
            FVM_Context *c = 0;
            unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
            swf[3] = (unsigned char)version;
            if (!(fvm_context_create(rt, &c) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK &&
                  fvm_avm1_set_url_rooted(c, dispatch_sink, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            unsigned ids[] = {1, 2, 3};
            FVM_Value value;
            for (dispatch_failure = 0; dispatch_failure < 3; ++dispatch_failure)
            {
                dispatch_coercion(c, ids);
                dispatch_coercion(c, ids + 1);
                dispatch_steps = dispatch_deliveries = 0;
                Program p;
                p.number(77);
                p.variable("url");
                p.variable("target");
                dispatch_dynamic(p, 194);
                dispatch_run(c, p, dispatch_failure ? FVM_EXCEPTION : FVM_OK);
                if (!(dispatch_steps == (dispatch_failure ? dispatch_failure : 2U) &&
                      dispatch_deliveries == (dispatch_failure ? 0U : 1U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!dispatch_failure)
                {
                    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_get_global(c, "deliveredTarget", &value) == FVM_OK &&
                          fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "t\0y", 3)))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            FVM_Object *root = 0;
            if (!(fvm_movieclip_create(c, "root", 3, &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            value = class_value(dispatch_body(c, dispatch_collect, 0));
            if (!(fvm_set_global(c, "collector", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program body;
            body.text("collected");
            body.number(0);
            body.text("collector");
            body.op(0x3d);
            body.op(0x1d);
            body.op(0);
            if (!(fvm_movieclip_define_frame(c, root, 2, "second", body.bytes.begin(), body.bytes.size()) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            for (unsigned throwing = 0; throwing < 2; ++throwing)
            {
                dispatch_failure = throwing ? 3 : 0;
                dispatch_steps = dispatch_collections = 0;
                dispatch_coercion(c, ids + 2);
                Program p;
                p.number(77);
                p.variable("request");
                dispatch_call(p);
                frame_dispatch_run(c, p, root, throwing ? 0 : 1, throwing ? FVM_EXCEPTION : FVM_OK);
                if (!(dispatch_steps == 1 && dispatch_collections == (throwing ? 0U : 1U)))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!throwing)
                {
                    if (!(fvm_object_get(c, root, "collected", &value) == FVM_OK && value.number == 17))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            dispatch_failure = 0;
            fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
